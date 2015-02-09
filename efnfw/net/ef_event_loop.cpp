#include "ef_event_loop.h"
#include "ef_connection.h"
#include "ef_operator.h"
#include "ef_net_settings.h"
#include "base/ef_utility.h"
#include "ef_net_log.h"
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>
#include <cassert>


namespace ef{

	EventLoop::EventLoop()
		:m_max_fds(NetSettings::epollSize),
		m_status(STATUS_CLOSED),
		m_epl(INVALID_SOCKET),
		m_ctlfd(INVALID_SOCKET),
		m_ctlfd1(INVALID_SOCKET),
		m_cur_id(1),m_id(-1),
		m_obj(NULL),m_clean(NULL)
	{
		mutexInit(&m_opcs);
#ifdef THREAD_SAFE
		mutexInit(&m_tmcs);
#endif
	}

	EventLoop::~EventLoop(){
		stop();
		while(m_status != STATUS_CLOSED){
			sleep_ms(50);
			addAsynOperator();
		}
		delAllOp();
		delAllConnections();
		sock_close(m_ctlfd);
		sock_close(m_ctlfd1);
		mutexDestroy(&m_opcs);
		if(m_clean)
			m_clean(m_obj);
	}

	int32 EventLoop::delAllConnections(){
		return 0;	
	}

	int32 EventLoop::getConId(){

		int32 id = atomicIncrement32((volatile int32*)&m_cur_id);
		id = atomicCompareExchange32((volatile int32*)&m_cur_id, 1, 0x7FFFFFFF);
		return id;
	}

	int32 EventLoop::startCtl(){
		int32 ret = 0;
		int32 sockets[2];
		if(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0){
			NLogError << "EventLoop" 
				<< std::hex << this 
				<< "create ctl sock fail!";
			return -1;	
		}
		m_ctlfd = sockets[0];
		m_ctlfd1 = sockets[1];
		setSocketNonblocking(m_ctlfd);
		setSocketNonblocking(m_ctlfd1);

		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.fd = m_ctlfd;
		ret = epoll_ctl(m_epl, EPOLL_CTL_ADD, m_ctlfd, &ev);
		//struct epoll_event ev1;
		//ev1.events = EPOLLIN;
		//ev1.data.fd = m_ctlfd;
		//ret = epoll_ctl(m_epl, EPOLL_CTL_ADD, m_ctlfd1, &ev1);

		if(ret < 0){
			sock_close(m_ctlfd);
			m_ctlfd = INVALID_SOCKET; 
			sock_close(m_ctlfd1);
			m_ctlfd1 = INVALID_SOCKET; 
		}
		return ret;
	}

	int32 EventLoop::addConnection(int32 id, Connection *con){
		assert(con);
		std::string addr;
		int32 port;
		con->getAddr(addr,port);
		con->setEventLoop(this);
		if(con->onCreate(this) < 0){
			NLogError << "EventLoop:" << this << ", con:" 
				<< con << ", id:" << id 
				<< ", fd:" << con->getFd() << ", " 
				<< (addr) << ":" << port << " onCreate fail!";
			con->recycle();
			return -1;
			
		}
		if(m_con_map.find(id) == m_con_map.end()){
			m_con_map.insert(con_map::value_type(id, con));
#if DETAIL_NET_LOG
			NLogTrace << "EventLoop:" << this << ", con:" 
				<< con << ", id:" << id 
				<< ", fd:" << con->getFd() << ", " 
				<< (addr) << ":" << port << " add sucess!";
#endif
		}else{
			NLogWarn << "EventLoop:" << this << ", con:" 
				<< con << ", id:" << id 
				<< ", fd:" << con->getFd() << ", " 
				<< (addr) << ":" << port << " has be added!";
		}
		return 0;
	}

	int32 EventLoop::delConnection(int32 id){
		std::string addr;
		int32 port; 
		con_map::iterator itor = m_con_map.find(id);
		Connection* con = NULL;
		if(itor != m_con_map.end()){
			con = itor->second;
			con->getAddr(addr,port);
			m_con_map.erase(itor);
#if DETAIL_NET_LOG
			NLogTrace << "EventLoop:" << this << ", con:" 
				<< con << ", id:" << con->getId() 
				<< ", fd:" << con->getFd() << ", " 
				<< (addr) << ":" << port << " del sucess!";
#endif
		}else{
			NLogError << "EventLoop:" << this
				<< ", id:" << id
				<< " del not find!";
		}
		return 0;
	}

	Connection* EventLoop::getConnection(int32 id){
		con_map::iterator itor = m_con_map.find(id);
		if(itor != m_con_map.end()){
			return (*itor).second;
		}
		return NULL;
	}

	int32 EventLoop::closeAllConnections(){
		std::list<Connection*> cons;
		con_map::iterator it = m_con_map.begin();
		for(; it != m_con_map.end(); ++it){
			cons.push_back(it->second);
		}
		std::list<Connection*>::iterator i = cons.begin();
		for(; i != cons.end(); ++i){
			(*i)->safeClose();
		}
		return 0;
	}


	int32 EventLoop::asynCloseAllConnections(){
		NetOperator* op = new CloseAllConnectionsOp();
		return addAsynOperator(op);
	}

	int32 EventLoop::stop(){
		NLogError << "EventLoop:" << std::hex << this << " stop!";
		int32 status = atomicCompareExchange32(&m_status, 
			STATUS_CLOSING, STATUS_RUNNING);
		if(status == STATUS_RUNNING){
			asynCloseAllConnections();
		} 
		return 0;
	}

	int32 EventLoop::init(){
		int32 ret = 0; 

		m_epl = epoll_create(m_max_fds); 
		if(m_epl < 0){
			NLogError << "EventLoop:" << std::hex 
				<< this << " init epoll_create fail!";
			return -1;

		}

		ret = startCtl();
		if(ret < 0){
			NLogError << "EventLoop:" << std::hex 
				<< this << " init startCtl fail!";
			return -4;
		}
		return 0;
	}

	size_t EventLoop::ConnectionsCount(){
		size_t i = 0;
		i = m_con_map.size();
		return i;
	}

	int32 EventLoop::run(){
		int32 ret = 0;
		int32 nfds = 0;
		m_status = STATUS_RUNNING;
		NLogError << "EventLoop:" << std::hex << this << " run start!";

		struct epoll_event events[MAX_PROC_EVENT_CNT];
		int32 events_on_loop = MAX_PROC_EVENT_CNT < NetSettings::procEventCnt ? 
				MAX_PROC_EVENT_CNT : NetSettings::procEventCnt;

		time_tv tv;
		while(1){
			//process Op and timer at first
			processOp();
			processTimer(tv);
			if(m_status != STATUS_RUNNING&& !ConnectionsCount()){
				m_status = STATUS_CLOSED;
				break;
			}
			if(tv.m_sec > 0 || (tv.m_sec == 0 && tv.m_usec > 0 )){
				//std::cout << "EventLoop:" << std::hex << this
				//	<< ", tv:" << tv.m_sec * 1000 + tv.m_usec / 1000
				//	<< std::endl;
				nfds = epoll_wait(m_epl,events, events_on_loop, 
						tv.m_sec * 1000 + tv.m_usec / 1000);
			}else{
				nfds = epoll_wait(m_epl,events, events_on_loop, -1);
			}
			if(nfds < 0){
				NLogTrace << "EventLoop:" << std::hex << this 
					<< ", epoll error,errno:" << errno;   
				if(errno != SOCK_EINTR){ 
					break;
				}
			}
			for(int32 i = 0; i < nfds && i < events_on_loop; ++i){
				if(events[i].data.fd == m_ctlfd){
					//std::cout << "epoll m_ctlfd"<< std::endl;
					continue;
				}
				std::string addr;
				int32 port;
				Device* con = (Device*)events[i].data.ptr;
				con->getAddr(addr,port); 
				if(events[i].events & EPOLLIN){
					ret = con->handleRead(this);
				}else if(events[i].events & EPOLLOUT){
					ret = con->handleWrite(this);
				}else{
					ret = -1;
				}

				if(ret < 0){
					NLogWarn << "EventLoop:" << std::hex << this 
							<< ", con:" << std::hex << con 
							<< std::dec << ", fd:" << con->getFd() 
							<< ", " << (addr) << ":" << port
							<< " handle event:" << events[i].events 
							<< " fail, recycle!";
					con->onFail();
					continue;
				}
#if DETAIL_NET_LOG
				else{
					NLogTrace << "EventLoop:" << std::hex << this 
						<< ", con:" << std::hex << con
						<< std::dec << ", fd:" << con->getFd() 
						<< ", " << (addr) << ":" << port
						<< " handle event success!";
				}
#endif
			}
		}
		
		NLogError << "EventLoop:" << std::hex << this << " run stop!";
		return 0;
	}

	int32 EventLoop::processOp(){
		NetOperator *op = NULL;
		char buf[1024];
		int32 loop = sizeof(buf);
		int32 cnt = 0;
		while(m_ops.size()){
			mutexTake(&m_opcs);
			if(m_ops.size() >= (int32)sizeof(op)){
				 m_ops.read((uint8*)&op, sizeof(op));
				mutexGive(&m_opcs);
			}else{
				mutexGive(&m_opcs);
				break;
			}
			//recv notify
			assert(op);
			op->process(this);
			delete op;
			++cnt;
		}
		while(loop >= (int32)sizeof(buf)){
			loop = recv(m_ctlfd, buf, sizeof(buf), 0);
		}

		return 0;
	}

	int32 EventLoop::delAllOp(){

		NetOperator *op = NULL;
		while(1){
			mutexTake(&m_opcs);
			if(m_ops.size() >= (int32)sizeof(op)){
				m_ops.read((uint8*)&op, sizeof(op));
				mutexGive(&m_opcs);
			}else{
				mutexGive(&m_opcs);
				break;
			}
			//recv notify
			assert(op);
			delete op;
		}

		return 0;
	}

	int32 EventLoop::addAsynOperator(NetOperator* op){

		int32 ret = 0;
		
		if(op){
			mutexTake(&m_opcs);
			m_ops.autoResizeWrite((const uint8*)&op, sizeof(op));
			mutexGive(&m_opcs);
		}

		char ctl;
		ret = send(m_ctlfd1, &ctl, sizeof(ctl), 0);
		if(ret < 0){
			NLogError << "EventLoop:" << std::hex 
				<< this << ", send ctl msg fail";
			return -1;
		}else{
#if DETAIL_NET_LOG
			NLogTrace << "EventLoop:" << std::hex 
				<< this << ", send ctl msg";
#endif
		}
		return ret;
	}

	int32 EventLoop::processTimer(time_tv &diff){
		timeval t;
		int32 ret = 0;
		gettimeofday(&t, NULL);
		time_tv tv(t);
		int32 max_timer_one_loop 
			= MAX_PROC_TIMER_CNT < NetSettings::procTimerCnt ?
			MAX_PROC_TIMER_CNT : NetSettings::procTimerCnt;
		timer_map::iterator itor = m_timer_map.begin();
		int32 i = 0;
		diff.m_sec = 0;
		diff.m_usec = 0;
		while(itor != m_timer_map.end() && i < max_timer_one_loop){
			time_tv tv1 = (*itor).first.tv;
			if(tv_cmp(tv, tv1) < 0){
				diff = tv_diff(tv1, tv);
				break;
			}
			timer_map::value_type t = (*itor);
#if DETAIL_NET_LOG
			int32 s = m_timer_map.size();
			NLogTrace << "EventLoop:" << std::hex 
				<< this << ", con:" << t.second.getConnection()
				<< std::dec
				<< ", timeout, key.tv:" << t.first.tv.m_sec
				<< ":" << t.first.tv.m_usec
				<< ", key.id:" << t.first.id
				<< ", key.con_id:" << t.first.con_id
				<< ", map.size:" << s;
#endif/*DETAIL_NET_LOG*/
			t.second.timeout(this);
			m_timer_map.erase(itor);
			itor = m_timer_map.begin();
			++i;
		}
		return ret;
	}


	int32 EventLoop::addNotify(Device *con, int32 noti){
		assert(con);
		std::string addr;
		int32 port;
		con->getAddr(addr,port);

		int32 ret = 0;
		struct epoll_event ev;
		ev.events = 0;
		//ev.events = EPOLLET;
		if(noti & READ_EVENT){
			ev.events |= EPOLLIN;
		}

		if(noti & WRITE_EVENT){
			ev.events |= EPOLLOUT;
		}

		ev.data.ptr = con;
		ret = epoll_ctl(m_epl, EPOLL_CTL_ADD, con->getFd(), &ev);

		if(ret >= 0){ 
#if DETAIL_NET_LOG 
			NLogTrace << "EventLoop:" << std::hex << this 
				<< ", con:" << std::hex << con 
				<< std::dec << ", fd:" << con->getFd()
				<< ", epoll_fd:" << m_epl << " " << (addr) 
				<< ":" << port << " setnoti:" << std::hex 
				<< noti << ", events:" << ev.events
				<< " success!";
#endif
		}else{
			NLogError << "EventLoop:" << std::hex << this 
				<< ", con:" << std::hex << con 
				<< std::dec << ", fd:" << con->getFd()
				<< ", epoll_fd:" << m_epl << " " << (addr) 
				<< ":" << port << " setnoti:" << std::hex 
				<< noti << ", events:" << " fail, errno:"
				<< errno;
		}
		return	ret;  
	}

	int32 EventLoop::modifyNotify(Device *con, int32 noti){
		assert(con);
		std::string addr;
		int32 port;
		con->getAddr(addr,port);

		int32 ret = 0;
		struct epoll_event ev;
		ev.events = 0;
		//ev.events = EPOLLET;
		if(noti & READ_EVENT){
			ev.events |= EPOLLIN;
		}

		if(noti & WRITE_EVENT){
			ev.events |= EPOLLOUT;
		}
		
		ev.data.ptr = con;
		ret = epoll_ctl(m_epl, EPOLL_CTL_MOD, con->getFd(), &ev);

		if(ret >= 0){  
#if DETAIL_NET_LOG
			NLogTrace << "EventLoop:" << std::hex << this 
				<< ", con:" << std::hex << con 
				<< std::dec << ", fd:" << con->getFd()
				<< ", epoll_fd:" << m_epl << " " << (addr) 
				<< ":" << port << " modifyNoti:" << std::hex 
				<< noti << ", events:" << ev.events
				<< " success!";
#endif
		}else{
			NLogError << "EventLoop:" << std::hex << this 
				<< ", con:" << std::hex << con 
				<< std::dec << ", fd:" << con->getFd()
				<< ", epoll_fd:" << m_epl << " " << (addr) 
				<< ":" << port << " modifyNoti:" << std::hex 
				<< noti << ", events:" << ev.events
				<< " fail, errno:" << errno;
		}
		return	ret;  
	}


	int32 EventLoop::clearNotify(Device *con){
		assert(con);
		std::string addr;
		int32 port;
		con->getAddr(addr,port);

		int32 ret = 0;
		struct epoll_event ev;
		ev.events = 0;

		ev.data.ptr = con;

		ret = epoll_ctl(m_epl, EPOLL_CTL_DEL, con->getFd(), &ev);

		if(ret >= 0){ 
#if DETAIL_NET_LOG 
			NLogTrace << "EventLoop:" << std::hex << this 
				<< ", con:" << std::hex << con 
				<< std::dec << ", fd:" << con->getFd()
				<< ", epoll_fd:" << m_epl
				<< " " << (addr) << ":"
				<< port << " claerNoti success";
#endif
		}else{
			NLogError << "EventLoop:" << std::hex << this 
				<< ", con:" << std::hex << con 
				<< std::dec << ", fd:" << con->getFd()
				<< ", epoll_fd:" << m_epl << " " << (addr) 
				<< ":" << port << " claerNoti fail, errno:"
				<< errno;
		}
		return	ret;  
	}

	int32 EventLoop::addTimer(Timer tm){
		TimerKey key;
		Connection *con = tm.getConnection();
		std::string addr;
		int32 port;

		key.tv = tm.getTimeoutTime();
		if(con){
			con->getAddr(addr,port); 
			key.con_id = con->getId();
		}else{
			key.con_id = 0;
		}
		key.id = tm.getId();
		int32 s = 0;
		timer_map::iterator itor = m_timer_map.find(key);
		if(itor != m_timer_map.end()){
			Connection *c = itor->second.getConnection();
			NLogError << "EventLoop:" << std::hex << this 
				<< ", con:" << std::hex << con 
				<< std::dec
				<< ", dumplicate timer:" << tm.getId()
				<< ", key.tv:" << key.tv.m_sec << ":"
				<< key.tv.m_usec << ", key.id:"
				<< tm.getId() << ", key.con_id:" << key.con_id
				<< ", timer.con:" << c << ", timer.con_id:"
				<< c->getId() << ", timer.fd:" << c->getFd();
				
		}
		m_timer_map[key] = tm;
		s = m_timer_map.size();

#if DETAIL_NET_LOG
		NLogTrace << "EventLoop:" << std::hex << this 
				<< ", con:" << std::hex << con 
				<< std::dec
				<< ", id:" <<  (con ? con->getId() : 0) 
				<< ", fd:," << (con ? con->getFd() : -1)
				<< " " << (con ? (addr) : "0")
				<< ":" << (con ? port : -1)
				<< ", add timer:" << tm.getId()
				<< ", key.tv:" << key.tv.m_sec << ":"
				<< key.tv.m_usec << ", key.id:"
				<< tm.getId() << ", key.con_id:" << key.con_id
				<< ", map.size:" << s;
#endif
		return 0;
	}

	int32 EventLoop::delTimer(Timer tm){
		TimerKey key;
		Connection *con = tm.getConnection();
		std::string addr;
		int32 port;
		int32 s = 0;

		con->getAddr(addr,port); 
		key.con_id = con->getId();
		key.id = tm.getId();
		key.tv = tm.getTimeoutTime();
		m_timer_map.erase(key);
		s = m_timer_map.size();
#if DETAIL_NET_LOG
		NLogTrace << "EventLoop:" << std::hex << this 
				<< ", con:" << std::hex << con << std::dec
				<< ", id:" <<  (con ? con->getId() : 0) 
				<< ", fd:," << (con ? con->getFd() : -1)
				<< " " << (con ? (addr) : "0")
				<< ":" << (con ? port : -1)
				<< " del timer:" << tm.getId()
				<< ", key.tv:" << key.tv.m_sec << ":"
				<< key.tv.m_usec << ", key.id:"
				<< tm.getId() << ", key.con_id:" << key.con_id
				<< ", map.size:" << s;
#endif
		return 0;
	}

	int32 EventLoop::asynAddConnection(int32 id, Connection* con){
		NetOperator *op = new AddConnectionOp(id, con);

		addAsynOperator(op);
#if DETAIL_NET_LOG
		NLogTrace << "EventLoop:" << std::hex << this
			<< ", con:" << std::hex << con << std::dec
			<< ", asynAddConnection, id:" << id
			<< ", fd:," << (con ? con->getFd() : -1)
			<< " " << (con ? (con->getIp()) : "0")
			<< ":" << (con ? con->getPort() : -1);
#endif
		return 0;

	}

	int32 EventLoop::asynCloseConnection(int32 id){
		NetOperator *op = new CloseConnectionOp(id);

		addAsynOperator(op);
#if DETAIL_NET_LOG
		NLogTrace << "EventLoop:" << std::hex << this
			<< std::dec
			<< ", asynCloseConnection, id:" << id;
#endif

		return 0;

	}

	int32 EventLoop::closeConnection(int32 id){
		int32 ret = 0;
		Connection *con = getConnection(id);
		if(con){
			ret = con->safeClose();
		}

		return ret;
	}

	int32 EventLoop::asynSendMessage(int32 id, const std::string &msg){
		NetOperator *op = new SendMessageOp(id, msg);
		addAsynOperator(op);

#if DETAIL_NET_LOG
		NLogTrace << "EventLoop:" << std::hex << this
			<< std::dec << ", asynSendMessage, id:" << id
			<< ", size:" << msg.size();
#endif
		return 0;
	}

	int32 EventLoop::sendMessage(int32 id, const std::string &msg){
		int32 ret = 0;
		Connection *con = getConnection(id);
		if(con){
			ret = con->sendMessage(msg);
			if(ret < 0){
				con->recycle();
			}
		}else{
			
			NLogError << "EventLoop:" << std::hex << this 
				<< ", con_id:" << std::dec << id 
				<< ", sendMesage not found!";
		}
		return 0;
	}

	int32 EventLoop::findDelEventLoopTimer(int32 id, Timer& tm){
		thread_timer_map::iterator itor = m_timers.find(id);
		if(itor != m_timers.end()){
			tm = itor->second;
			m_timers.erase(itor);
			return  0;
		}
		return  -1;

	}

	int32 EventLoop::startEventLoopTimer(int32 id, 
			int32 timeout, 
			TimerHandler* handler){
		int32   ret = 0; 
		Timer   tm; 
		ret = findDelEventLoopTimer(id, tm); 
		if(ret == 0){ 
			delTimer(tm); 
		} 

		timeval tv;
		gettimeofday(&tv, NULL);
		tv.tv_sec += timeout / 1000;
		tv.tv_usec += timeout % 1000 * 1000;
		Timer   tm1(NULL, id, tv, handler);
		m_timers[id] = tm1;
		return addTimer(tm1);
	}

	int32 EventLoop::stopEventLoopTimer(int32 id){
		int32   ret = 0;
		Timer   tm;
		ret = findDelEventLoopTimer(id, tm);
		if(ret == 0){
			delTimer(tm);
		}
		return  0;
	}

}

