#include "net/ef_sock.h"
#include "proto/msg_head.h"
#include "proto/connect_server.pb.h"
#include "proto/peer_server.pb.h"
#include "base/ef_utility.h"
#include "base/ef_log.h"
#include "base/ef_aes.h"
#include "base/ef_md5.h"
#include <iostream>
#include <sys/epoll.h>
#include <sstream>
#include <fstream>
#include <string>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <map>

using namespace ef;
using namespace std;
using namespace gim;


#define isBigEndian()   ((*(unsigned short *) ("KE") >> 8) == 'K')
#define rol2(x,n)( ((x) << (n)) | ((x) >> (64-(n))) )
#define swap8(b) ( (rol2((uint64_t)(b),8 )&0x000000FF000000FFULL) | \
                   (rol2((uint64_t)(b),24)&0x0000FF000000FF00ULL) | \
                   (rol2((uint64_t)(b),40)&0x00FF000000FF0000ULL) | \
                   (rol2((uint64_t)(b),56)&0xFF000000FF000000ULL) )
#ifndef htonll
#define htonll(b) ( isBigEndian()?(b):swap8(b) )
#endif
#ifndef ntohll
#define ntohll(b) ( isBigEndian()?(b):swap8(b) )
#endif

#define STATUS_OK (0)
#define STATUS_SOCKET_ERR (-1)
#define STATUS_OTHER_ERR  (-1000)
#define KEEPALIVE_SPAN	  (60 * 1000 * 3)

enum{
	CLIENT_STATUS_INIT = 0,
	CLIENT_STATUS_WAIT_RESP = 1,
};

#define LOG_OUT std::cout << ef::getStrTimeAndPid(time(NULL)) \
		<< "<sn:" << lastSn() << "> "

#define LOG_OUT_NO_SN std::cout << ef::getStrTimeAndPid(time(NULL))

class Client{
public:
	Client(int32 fd = -1):m_fd(fd), m_sn(0), 
		m_status(CLIENT_STATUS_INIT){
	}

	~Client(){
		if(m_fd >= 0){
			closesocket(m_fd);
		}
	}
	
        int32   const_req(int c, std::string &req, const std::string &body){

                int32   len = 4 + 4 + 4 + body.size();
                uint32  magic = htonl(0x20140417);
		uint32  cmd = htonl(c);
                uint32  nlen = htonl(len);

                req.append((char*)&magic, 4);
                req.append((char*)&nlen, 4);
		req.append((char*)&cmd, 4);
                req.append(body.data(), body.size());

                return  (int32)len;
        }

	int32	const_service_req(const ServiceRequest& srreq, 
			std::string& req){
		return srreq.SerializeToString(&req);
	}

	int32	parse_service_resp(const std::string& resp, 
			ServiceResponse& svresp){
		return svresp.ParseFromString(resp);
	}

	int32	login(const std::string& cid, const std::string& pwd, 
		int enc, const std::string& version){
		int32 ret = 0;
		LoginRequest lgr;
		lgr.set_cid(cid);		
		lgr.set_enc(enc);
		lgr.set_version(version);	
	
		std::string s = cid + pwd + version + ef::itostr(time(NULL));
		std::string tk;	
		ef::MD5Hex(tk, (const ef::uint8*)s.data(), s.size());
		lgr.set_token(tk);

		m_cid = cid;
		m_pwd = pwd;
		LoginResponse lgresp;
		std::string req;
		lgr.SerializeToString(&req);	
	
		std::string rsp;
		ret = send_req(100, req);

                if(ret < 0){
                        LOG_OUT << "test_login send fail, cid:" << m_cid 
				<< std::endl;
                        return  ret;
                }
		
		int32 cmd = 0;
		ret = recv_resp(cmd, rsp);	
		if(ret <= 0){
			LOG_OUT << "login recv_resp fail, cid:" << m_cid
				<< std::endl;
			return ret;
		}
		//redirect
		if(cmd == 1001){
			RedirectResponse rdresp;
			rdresp.ParseFromString(rsp);
			disconnect();
			if(rdresp.status() != 0 || !rdresp.addrs_size()){
				return -1;
			}
			int i = 0;
			for(; i < rdresp.addrs_size(); ++i){
				const Address& a = rdresp.addrs(i);
				LOG_OUT << "test_login redirect, cid:"
					<< m_cid << ", addr:" << a.ip()
					<< ", port:" << a.port() << std::endl;
				ret = bind_connect(a.ip(), a.port(), 
					m_local_ip, m_local_port);
				if(ret < 0){
					LOG_OUT << "test_login redirect fail, cid:"
						<< m_cid << std::endl;
				}else{
					break;
				}
			}
			if(i < rdresp.addrs_size())
				return	login(m_cid, pwd, enc, version);
		}

		lgresp.ParseFromString(rsp);
		if(lgresp.status() == 0){
			LOG_OUT << "test_login success: cid:" << m_cid 
				<< ", sessid:" << lgresp.sessid() << std::endl;	
		}else{
			LOG_OUT << "test_login fail: cid:" << m_cid 
				<< ", status:" 
				<< lgresp.status() << std::endl;	
			return -1;
		}
		m_sessid = lgresp.sessid();
		++success_count;
		return	ret;
	}

	int32	keepAlive(){
		int32 ret = 0;
		if(gettime_ms() - m_send_time < KEEPALIVE_SPAN){
			LOG_OUT << "cid:" << m_cid
				<< "keepAlive not send!" << std::endl;
			return	0;
		}
		ret = send_req(0, "");
		if(ret < 0){
			LOG_OUT << "cid:" << m_cid
				<< "keepAlive send fail!" << std::endl;
			return  ret;
		}else{
			LOG_OUT << "cid:" << m_cid
				<< " keepAlive!" << std::endl;
		}
		return  ret;
	}


	int32	get_messages(){
		int32 ret = 0;
		
	}


	int32	send_peer_message(const std::string& to, const std::string& data){
		int32 ret = 0;
		PeerPacket ppack;	
		ppack.set_cmd(110);
		SendPeerMessageRequest *smreq = ppack.mutable_send_peer_msg_req();	
		Message* pm = smreq->mutable_msg();
		pm->set_from(m_cid);
		pm->set_to(to);
		pm->set_data(data);
		string payload;
		ppack.SerializeToString(&payload);
		ret = do_service_req(200, data);	

		return ret;
	}



	int32	do_service_req(int32 type, const string& payload){
		int32 ret = 0;
		std::string req;
		ServiceRequest srreq;
		srreq.set_svtype(type);
		srreq.set_payload(payload);
		srreq.set_from_sessid(m_sessid);
		srreq.set_sn(lastSn());
		const_service_req(srreq, req);	
		std::string rsp;
		ret = send_req(200, req);
	
                if(ret < 0){
                        LOG_OUT << "cid:" << m_cid 
				<< "test_service send fail!" << std::endl;
                        return  ret;
                }
		return	ret;
			
	}

	int32	service(int32 argc, const char** argv){
		int32 ret = 0;
		std::string payload;
		ret = get_payload(argc, argv, payload);
		if(ret >=0)
			ret = do_service_req(atoi(argv[0]), payload);
		else
			LOG_OUT << "cid:" << m_cid
				<< "get_payload fail!" << std::endl;	
		return	ret;
	
	}

        int32   disconnect(){
                if(m_fd != INVALID_SOCKET){
                        closesocket(m_fd);
                        m_fd = INVALID_SOCKET;
                }
                return  0;
        }	

        int32   do_recv(std::string  &msg){

                uint32  totallen = 0;
                int     ret = 0;
                int     rcvcnt = 0;

                msg.resize(12);
                char* buf = (char*)msg.data();
		
		while(rcvcnt < 12){
			ret = recv(m_fd, buf + rcvcnt, 12 - rcvcnt, 0);
			if(ret <= 0){
				return -1;
			}
			rcvcnt += ret;
		}

                totallen = *(uint32*)(buf + 4);
                totallen = ntohl(totallen);
                rcvcnt = 12;

                msg.resize(totallen);
                buf = (char*)msg.data() + 12;

                while(rcvcnt < totallen){
                        ret = recv(m_fd, buf, totallen - rcvcnt, 0);

                        if(ret <= 0){
                                LOG_OUT << "server addr:" << m_server_addr
                                        << ", port:" << m_server_port 
                                        << "recv body fail, errno:" << strerror(errno)
					<< std::endl;
                                return  STATUS_SOCKET_ERR;
                        }
                        rcvcnt += ret;
                        buf += ret;
                }
                return  totallen;

        }


	int32	handle_peer_message_req(const std::string& sn,const PeerPacket& pack,
			std::string& resppayload){
		int32 ret = 0;
		if(!pack.has_send_peer_msg_req()){
			LOG_OUT_NO_SN << "cid:" << m_cid << ", sn:" << sn
				<< " handle_peer_message_req miss send_peer_msg_req\n";	
			return -101;
		}

		const SendPeerMessageRequest& smreq = pack.send_peer_msg_req();
		const Message& pm = smreq.msg();
		LOG_OUT_NO_SN << "cid:" << m_cid << ", sessid:"
			<< m_sessid << ", sn:" << sn 
			<< ", handle_peer_message_req, id:"
			<< pm.id() << ", time:" << pm.time()
			<< ", from:" << pm.from()
			<< ", data:" << pm.data() 
			<< ", delay:" << gettime_ms() - pm.time() << std::endl;

		PeerPacket resppack;
		resppack.set_cmd(115);
		RecvPeerMessageResponse* rmresp = resppack.mutable_recv_peer_msg_resp();

		*(rmresp->mutable_msg()) = (pm); 		
		resppack.SerializeToString(&resppayload);

		return ret;	
	}

	int32	handle_peer_message_resp(const PeerPacket& pack){
		int32 ret = 0;
		if(!pack.has_send_peer_msg_resp()){
			LOG_OUT << "cid:" << m_cid
				<< " handle_peer_message_resp miss send_peer_msg_resp\n";	
			return -101;
		}		

		const SendPeerMessageResponse& smresp = pack.send_peer_msg_resp();
		LOG_OUT << "cid:" << m_cid << ", sessid:"
			<< m_sessid << ", handle_peer_message_resp"
			<< std::endl; 

		return ret;
	}


	int32	handle_get_peer_message_resp(const PeerPacket& pack){
		int32 ret = 0;
		if(!pack.has_get_peer_msg_resp()){
			LOG_OUT << "cid:" << m_cid
				<< " handle_get_message_resp miss get_peer_msg\n";	
			return -101;
		}	

		stringstream os;
		const GetPeerMessageResponse& gmresp = pack.get_peer_msg_resp();
		int64 lastmsgid = gmresp.last_msgid();	

		os << " { last_msgid:" << lastmsgid << "; msgs[";
		for(size_t i = 0; i < gmresp.msgs_size(); ++i){
			const Message& m = gmresp.msgs(i);
			os << "{ msgid:" << m.id() << ", from:" << m.from()
				<< ", to:" << m.to() << ", time:" << m.time() 
				<< ", data:" << m.data() << "};";
		}	

		os << "] }";
		LOG_OUT << "cid:" << m_cid << ", sessid:"
			<< m_sessid << ", handle_get_peer_message_resp :"
			<< os.str() << std::endl;
	
		return ret;	
	}

	int32	handle_peer_request(const std::string& sn, 
		const std::string& payload, std::string& outpayload){
		int32 ret = 0;
		PeerPacket reqpack;
		if(!reqpack.ParseFromString(payload)){
			LOG_OUT_NO_SN << "cid:" << m_cid << ", sn:" << sn
				<< " handle_peer_request ParseFromString fail\n";
			return -101;
		}
		switch(reqpack.cmd()){
		case 110:
			ret = handle_peer_message_req(sn, reqpack, outpayload);		
			break;
		case 112:
			break;
		}

		return 0;		
	}

	int32	handle_peer_response(const std::string& payload){
		int32 ret = 0;
		PeerPacket reqpack;
		if(!reqpack.ParseFromString(payload)){
			LOG_OUT << "cid:" << m_cid
				<< " handle_peer_response ParseFromString fail\n";
			return -101;
		}
		switch(reqpack.cmd()){
		case 111:
			ret = handle_peer_message_resp(reqpack); 
			break;
		case 113:
			ret = handle_get_peer_message_resp(reqpack);
			break;
		}
		return 0;
	}

	int32   handle_service_request(const std::string& resp){
		int32 ret = 0;
		ServiceRequest svreq;
		std::string outpayload;
		++total_recv_req;
		if(!svreq.ParseFromString(resp)){
			LOG_OUT << "cid:" << m_cid
				<< " handle_service_request ParseFromString fail\n";
			return -1;
		}
		if(svreq.from_sessid() != m_sessid){
			LOG_OUT << "cid:" << m_cid
				<< " handle_service_request,ret sessid:" 
				<< svreq.from_sessid()
				<< " != m_sessid:" << m_sessid << std::endl;
			ret = -200;				
		}		
		switch(svreq.svtype()){
		case 200:
			ret = handle_peer_request(svreq.sn(), svreq.payload(), outpayload);
			break;	
		}
exit:
		ServiceResponse svresp;
		svresp.set_sn(svreq.sn());
		svresp.set_from_sessid(m_sessid);
		svresp.set_to_sessid(svreq.from_sessid());
		svresp.set_svtype(svreq.svtype());
		svresp.set_status(ret);
		svresp.set_payload(outpayload);	
		std::string msg;
		std::string body;
		svresp.SerializeToString(&body);
		ret = send_req(201, body);
		if(ret == 0)
			++recv_req_success_count;
		m_status = CLIENT_STATUS_INIT;
		return ret;	
	}

	int32	handle_service_response(const std::string& resp){
		int32 ret = 0;
		ServiceResponse svresp;
		if(!svresp.ParseFromString(resp)){
			LOG_OUT << "cid:" << m_cid 
				<< " handle_service_response ParseFromString fail\n";
			return -1;
		}
		
		switch(svresp.svtype()){
		case 200:
			ret = handle_peer_response(svresp.payload());
		}
		if(svresp.status()){
			LOG_OUT << "cid:" << m_cid
				<< " handle_service_response fail, status:" 
				<< svresp.status() << std::endl;
		}else{
			std::stringstream os;
			if(lastSn() == svresp.sn()){
				++success_count;
			}else{
				LOG_OUT << "SN_ERROR, cid:" << m_cid 
					<< " service_resp resp sn:" << svresp.sn()
					<< " req sn:" << lastSn() << std::endl;	
			}
		}
		++m_sn;
		return 0;
	}


	std::string lastSn(){
		std::stringstream os;
		os << m_cid << "." << m_sn;
		return os.str();
	}


	int64	lastSendTime(){
		return m_send_time;
	}

	int32	handle_resp(){
		int32 cmd = 0;
		int32 ret = 0;
		std::string resp_body;
		int64 n = gettime_ms();
		while(sockGetPending(m_fd) >= 12){
			ret = recv_resp(cmd, resp_body);
			if(ret < 0){
				LOG_OUT << "handle_resp recv_resp fail\n";
				return ret;
			}
			switch(cmd){
			case SERVICE_REQ:
				total_spend_time -= (n - m_send_time);
				ret = handle_service_request(resp_body);
				break;
			case SERVICE_RESP:
				ret = handle_service_response(resp_body);	
			}
			if(ret < 0)
				LOG_OUT << "cid:" << m_cid 
					<< " handle event fail, delete\n";
		}
		return ret;	
	}

        int32   recv_resp(int32& cmd, std::string &resp){

                int32	ret = 0;
                uint32  totallen = 0;
                std::string str;
		int64 n = gettime_ms();
                ret = do_recv(str);

                if(ret < 0){
                        LOG_OUT << " server addr:" << m_server_addr
                                << ", port:" << m_server_port 
                                << " recv resp fail\n";
                        return  ret;
                }
		++total_resp;	
		total_spend_time += n - m_send_time;
		head h = *(head*)&str[0];
		cmd = htonl(h.cmd);
		m_status = CLIENT_STATUS_INIT;
					
		resp = str.substr(12);

                return  ret;
        }

        int32   send_req(int cmd, const std::string& req){
                std::string msg;

                const_req(cmd, msg, req);
		
                int ret = send(m_fd, msg.data(), msg.size(), 0);
		if(ret = 0)
			return -1;
		m_send_time = gettime_ms();
		++total_req;
		m_status = CLIENT_STATUS_WAIT_RESP;
                return  ret;
        }	

	int32	bind_connect(const  std::string& serverip, int32 serverport,
			const std::string& localip, int32 localport){
		
		if(localport){
			m_fd = tcpBindConnect(const_cast<char*>(serverip.data()), serverport, 
			const_cast<char*>(localip.data()), localport);
		}else{
			m_fd = tcpConnect(const_cast<char*>(serverip.data()), serverport, 
				NULL, localport);
		}
		if(m_fd < 0){
			LOG_OUT << "bind_connect fail, serverip:" << serverip
				<< ", serverport:" << serverport
				<< ", localip:" << localip << ", localport:"
				<< localport << std::endl;
			return -1;
		}
		m_server_addr = serverip;
		m_server_port = serverport;
		m_local_ip = localip;
		m_local_port = localport;
		return m_fd;
	}

	int32 get_payload(int argc, const char** argv, string& payload){
		int32 ret = 0;
		if(strcmp(argv[0], "200") == 0){
		 	if(strcmp(argv[1], "send_peer_msg") == 0 && argc >= 4){
				ret = get_send_peer_msg_payload(argv[2], argv[3], payload);	
			}else if(strcmp(argv[1], "get_peer_msg") == 0 && argc >= 4){
				ret = get_get_peer_msg_payload(atoi(argv[2]), atoi(argv[3]), payload);
			}
		}
		return ret;
	}

	int get_send_peer_msg_payload(const string& to, const string& data, string& payload){
		PeerPacket pack;
		pack.set_cmd(110);
		SendPeerMessageRequest* smreq = pack.mutable_send_peer_msg_req();		
		Message* pm = smreq->mutable_msg();
		pm->set_from(m_cid);
		pm->set_to(to);
		pm->set_data(data);
		pm->set_time(gettime_ms());
		pack.SerializeToString(&payload);

		return 0;
	}

	int get_get_peer_msg_payload(int32 startmsgid, int32 count, string& payload){
		PeerPacket pack;
		pack.set_cmd(112);
		GetPeerMessageRequest* smreq = pack.mutable_get_peer_msg_req();		
		smreq->set_cid(m_cid);
		smreq->set_start_msgid(startmsgid);
		smreq->set_count(count);
		pack.SerializeToString(&payload);

		return 0;
	}

	int32	getlocalport(){
		struct sockaddr_in a;
		memset(&a, 0, sizeof(a));
		socklen_t len = sizeof(a);
		getsockname(m_fd, (sockaddr*)&a, &len);
		return	a.sin_port;
	}

	SOCKET getFd(){
		return m_fd;
	}

	bool isIdle(){
		return m_status == CLIENT_STATUS_INIT;
	}

	int32 localPort(){
		if(m_local_port)
			return m_local_port;
		return 	getlocalport();
	}

private:
	friend	class CliSet;
	std::string	m_server_addr;
	int32		m_server_port;
	std::string	m_local_ip;
	int32		m_local_port;
	std::string	m_cid;
	std::string	m_pwd;
	int32		m_sn;
	int64 		m_send_time;
	int32		m_status;
	std::string	m_sessid;
	SOCKET	m_fd;
	static	int32	total_req;
	static  int32	total_resp;
	static	int32	last_total_req;
	static	int32	last_total_resp;
	static	int32	success_count;
	static	int32	total_recv_req;
	static	int32	recv_req_success_count;
	static	int64	total_spend_time;
	static	int64	last_total_spend_time;
}; 


int32	Client::total_req = 0;
int32	Client::total_resp = 0;
int32	Client::last_total_req = 0;
int32	Client::last_total_resp = 0;
int32	Client::success_count = 0;
int32	Client::total_recv_req = 0;
int32	Client::recv_req_success_count = 0;
int64	Client::total_spend_time = 0;
int64	Client::last_total_spend_time = 0;

class CliSet{
public:
	CliSet():m_run(false){
	}

	int32 init(const std::string& serverip, int32 serverport, 
		const std::string& localip, int32 startlocalport,
		int32 max_con, int32 tps, const std::string& cid,
		const std::string& pwd, int32 enc, 
		const std::string& version){
		m_server_addr = serverip;
		m_server_port = serverport;
		m_local_ip = localip;
		m_local_port = startlocalport;
		m_start_port = startlocalport;
		m_epoll_fd = epoll_create(65535);
		m_max_cnt = max_con;
		m_cid = cid;
		m_pwd = pwd;
		m_tps = tps;
		m_max_port = 0;
		m_enc = enc;
		m_version = version;
		return 0;
	}

	int32 connect_server(){
		Client* c = new Client();
		int i = 0;
		int32 ret = 0;
		while(m_local_port < 65536 && c->bind_connect(m_server_addr, 
			m_server_port, m_local_ip, m_local_port) < 0 && i < 20){
			++i;
			if(m_local_port){
				++m_local_port;
			}
		}
		m_local_port;
		if(i >= 20){
			delete c;
		}else{
			std::stringstream os;
			if(m_max_cnt > 1){
				os << m_cid << "." <<  m_local_ip << "." << m_local_port;
			}else{
				os << m_cid;
			}
			ret = c->login(os.str(), m_pwd, m_enc, m_version); 
			if(ret < 0){
				LOG_OUT_NO_SN << "cid:" << os.str() << " login fail!"
					<< std::endl;
				delete c;
				return 0;
			}
			struct epoll_event ev;
			ev.events=EPOLLIN;
			ev.data.ptr=c;
			ret = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, c->getFd(), &ev);
			if(ret < 0){
				LOG_OUT_NO_SN << "cid:" << os.str() << " epoll_ctl fail!"
					<< std::endl;
				delete c;
				return 0;
			}
			m_con_map[c->localPort()] = c;
			if(m_local_port)
				++m_local_port;
			m_max_port = c->localPort() > m_max_port ? c->localPort() : m_max_port;
		}
		return 0;
	}

	int32 run(int32 argc, const char** argv){
		m_run = true;
		const int32 events_on_loop = 128;
		const int32 slp_ms = 10;
		int32 cnt = 0;
		int32 kpcnt = 0;
		int32 idx = 0;
		int32 kplvidx = 0;
		struct epoll_event events[events_on_loop];
		int64 lastms = gettime_ms();
		while(m_run){	
			srand(lastms);
			int64 n = gettime_ms();
			if(m_con_map.size() < m_max_cnt && m_local_port < 65536)
				connect_server();
			int32 nfds = epoll_wait(m_epoll_fd, events, events_on_loop, slp_ms);
			if(nfds < 0){
				LOG_OUT_NO_SN << "epoll_wait fail!\n";
			}
			for(int i = 0; i < nfds; ++i){
				Client* c = (Client*)events[i].data.ptr;
				int32 ret = c->handle_resp();
				if(ret < 0){
					struct epoll_event ev;
					ev.events=EPOLLIN|EPOLLET;
					ev.data.ptr=c;
					epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, c->getFd(), &ev);
					m_con_map.erase(c->localPort());
					delete c;
				}
			}
			int32 k = (n - lastms) * m_tps / 1000;
			int32 sz = m_con_map.size();
			for(int j = 0; cnt < k && j < 10 && j < sz; ++j){
				++idx;
				Client* tc = NULL;
				int32 key = idx % (m_max_port - m_start_port + 1) + m_start_port;
				std::map<int32, Client*>::iterator it = m_con_map.find(key);
				if(it != m_con_map.end()){
					tc = it->second;
				}
				if(!tc)
					continue;
				if(tc->isIdle())
					tc->service(argc, argv);
				++cnt;
				if(n - tc->lastSendTime() > 1000){
					LOG_OUT_NO_SN << "sn:" << tc->lastSn() 
						<< ", send at:" << tc->lastSendTime()
						<< " more than 1000 ms\n";
				}
			}
			//5 min keep alive
			int32 m = (n - lastms) * sz / KEEPALIVE_SPAN;
			for(int l = 0; kpcnt < m && l < 10 && l < m; ++l){
				++kplvidx;
				Client* tc = NULL;
				int32 key = kplvidx % (m_max_port - m_start_port + 1) + m_start_port;
				//LOG_OUT_NO_SN << "key:" << key << ", max_port:" << m_max_port 
				//	<< ", m_start_port:" << m_start_port << std::endl;
				std::map<int32, Client*>::iterator it = m_con_map.find(key);
				if(it != m_con_map.end()){
					tc = it->second;
				}
				if(!tc)
					continue;
				if(tc->isIdle()){
					tc->keepAlive();
					++kpcnt;
				}
			}

			if(n - lastms > 1000){
				int32 req_tps = Client::total_req - Client::last_total_req;
				int32 resp_tps = Client::total_resp - Client::last_total_resp;
				int64 total_ms = Client::total_spend_time - Client::last_total_spend_time;
				Client::last_total_req = Client::total_req ;
				Client::last_total_resp = Client::total_resp;
				Client::last_total_spend_time = Client::total_spend_time;
				LOG_OUT_NO_SN << "total req:" << Client::total_req
					<< ", total resp:" << Client::total_resp
					<< ", success count:" << Client::success_count
					<< ", req tps:" << req_tps
					<< ", resp tps:" << resp_tps
					<< ", avg time:" << (resp_tps ? (total_ms / resp_tps) : 0)
					<< ", total_recv_req:" << Client::total_recv_req
					<< ", recv_req_success_count:" << Client::recv_req_success_count
					<< std::endl;
				lastms = n;
				cnt = 0;
				kpcnt = 0;
			}	
		}
	}

private:
	std::string     m_server_addr;
	int32           m_server_port;
	std::string     m_local_ip;
	int32		m_start_port;
	int32           m_local_port;
	int32		m_max_port;
	std::string	m_cid;
	std::string	m_pwd;
	int32		m_epoll_fd;
	int32		m_run;
	int32		m_tps;
	int32		m_max_cnt;
	int32		m_enc;
	std::string	m_version;
	std::map<int32, Client*> m_con_map;
};


int main(int argc, const char** argv){
	if(argc < 12){
		LOG_OUT_NO_SN << "test <addr> <port> <local_ip> <start_port> <count> <tps> "
			"<cid> <password> <enc> <version> \n"
			"200 <send_peer_msg> <to> <data>\n"
			"200 <get_peer_msg> <starmsgid> <count>\n"
			<< std::endl;
		return	0;
	}
	int ret = 0;
	int l = 0;
	CliSet clst;
	clst.init(argv[1], atoi(argv[2]), argv[3], atoi(argv[4]), 
		atoi(argv[5]), atoi(argv[6]), 
		(argv[7]), (argv[8]), atoi(argv[9]), argv[10]);
	clst.run(argc - 11, argv + 11);
	return	0;
}
