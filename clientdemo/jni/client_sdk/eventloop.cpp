#include "eventloop.h"
#include "client_conn.h"
#include "opbase.h"
#include "client_log.h"

namespace gim
{	
	//Eventloop
	EventLoop::EventLoop()
		:m_run(false)
	{
	}
	EventLoop::~EventLoop()
	{
	}

	int32 EventLoop::startCtl()
	{
		mutex_init(&m_ops_mtx);
		int32 sockets[2];
		if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0)
		{
			return -1;
		}
		m_ctlfdr = sockets[0];
		m_ctlfdw = sockets[1];
		set_socket_nonblocking(m_ctlfdr);
		set_socket_nonblocking(m_ctlfdw);
		return 0;
	}

	int32 EventLoop::startLoop()
	{
		startCtl();
		if (!m_run)
		{
			m_run = true;
			int32 ret = ef::thread_create(&m_thread, NULL, (PTHREAD_FUNC)workThreadProcess, this);
			return ret;
		}
	}
	int32 EventLoop::onStopAndWait()
	{
		if (m_run)
		{
			thread_join(&m_thread);
		}
		stopCtl();
		mutex_destroy(&m_ops_mtx);
	}
	int32 EventLoop::run()
	{
		SDK_LOG(LOG_LEVEL_TRACE, "eventloop run");
		struct timeval tv;
		struct timeval* ptv;
		fd_set fds;

		while (m_run)
		{
			processTimers(tv);
			ptv = (tv.tv_sec == 0 && tv.tv_usec == 0) ? NULL : &tv;
			FD_ZERO(&fds);
			SOCKET maxfd = m_ctlfdr;
			FD_SET(m_ctlfdr, &fds);
			for (CliConnMap::iterator it = m_conns.begin(); it != m_conns.end();it++)
			{
				CliConn* pcon = it->second;
				if (pcon && pcon->getfd() != INVALID_SOCKET)
				{
					FD_SET(pcon->getfd(), &fds);
					maxfd = (maxfd >= pcon->getfd()) ? maxfd:pcon->getfd();
				}
			}
			maxfd++;
			//SDK_LOG(LOG_LEVEL_TRACE, "select time out = %s", itostr(tv.tv_sec * 1000 * 1000 + tv.tv_usec).c_str());
			int32 ret = select(maxfd, &fds, NULL, NULL, ptv);
			if (ret < 0)
			{
				if (ret != /*SOCK_EINTR*/4)
				{
					SDK_LOG(LOG_LEVEL_TRACE, "select error %d", ret);
					return -1;
				}
			}
			else if (ret == 0)
			{
				//time out
			}
			else
			{
				if (FD_ISSET(m_ctlfdr, &fds))
				{
					processOps();
				}

				std::vector<std::string> errconns;
				for (CliConnMap::iterator it = m_conns.begin(); it != m_conns.end(); it++)
				{
					CliConn* pcon = it->second;
					if (pcon && FD_ISSET(pcon->getfd(), &fds))
					{
						if (pcon->handleRead() < 0)
						{
							pcon->onDisconnect();
							errconns.push_back(pcon->getCid());
						}
					}
				}
				for (std::vector<std::string>::iterator it = errconns.begin(); it != errconns.end(); ++it)
				{
					delConn(*it);
				}
			}
		}
		//onStopAndWait();
		SDK_LOG(LOG_LEVEL_TRACE, "eventloop exit");
		return 0;
	}
	int32 EventLoop::workThreadProcess(EventLoop* el)
	{
		return el->run();
	}
	int32 EventLoop::stopCtl()
	{
		if (m_ctlfdw != INVALID_SOCKET)
			sock_close(m_ctlfdw);
		if (m_ctlfdr != INVALID_SOCKET)
			sock_close(m_ctlfdr);
	}
	int32 EventLoop::processOps()
	{
		SDK_LOG(LOG_LEVEL_TRACE, "EventLoop::processOps");
		Op *op = NULL;
		char buf[1024];
		int32 loop = sizeof(buf);
		while (m_ops.size()){
			mutex_take(&m_ops_mtx);
			if (m_ops.size() >= (int32)sizeof(op)){
				m_ops.read((uint8*)&op, sizeof(op));
				mutex_give(&m_ops_mtx);
			}
			else{
				mutex_give(&m_ops_mtx);
				break;
			}
			if (op)
			{
				SmartOp sp(op);
				sp->process(this);
				if (sp.release() > 0 && sp.get())
				{
					CliConnMap::iterator it = m_conns.find(sp->getCid());
					if (it != m_conns.end())
					{
						CliConn* conn = it->second;
						conn->addTimer(sp->getSN(), sp);
					}
				}
			}
		}
		while (loop >= (int32)sizeof(buf)){
			loop = recv(m_ctlfdr, buf, sizeof(buf), 0);
		}

		return 0;
	}
	int32 EventLoop::processTimers(struct timeval& tv)
	{
		timeval tnow;
		gettimeofday(&tnow, NULL);
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		for (CliConnMap::iterator it = m_conns.begin(); it != m_conns.end(); it++)
		{
			CliConn* pcon = it->second;
			if (pcon)
			{
				timeval tvtem;
				tvtem.tv_sec = 0;
				tvtem.tv_usec = 0;
				pcon->processTimers(tnow, tvtem);
				if (!(tvtem.tv_sec == 0 || tvtem.tv_usec == 0) && tv_cmp(tv, tvtem) < 0)
				{
					tv = tvtem;
				}
			}
		}
	}
	int32 EventLoop::asynAddOp(Op* op)
	{
		if (m_ctlfdw != INVALID_SOCKET)
		{
			if (op)
			{
				mutex_take(&m_ops_mtx);
				m_ops.auto_resize_write((const uint8*)&op, sizeof(op));
				mutex_give(&m_ops_mtx);
			}

			char ctl = 0;
			int32 ret = send(m_ctlfdw, &ctl, sizeof(ctl), 0);
			return ret;
		}
		return -1;
	}
	int32 EventLoop::asynStop()
	{
		if (m_run)
		{
			m_run = false;
			asynAddOp(NULL);
			onStopAndWait();
		}
		return 0;
	}
	CliConn* EventLoop::findConn(const std::string& cid)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "EventLoop::findConn cid=%s", cid.c_str())
		CliConnMap::iterator it = m_conns.find(cid);
		if (it != m_conns.end())
		{
			return it->second;
		}
		return NULL;
	}
	CliConn* EventLoop::addConn(const std::string& cid)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "EventLoop::addConn cid=%s", cid.c_str())
		CliConn* conn = findConn(cid);
		if (conn)
		{
			return conn;
		}

		conn = new CliConn();
		std::pair<CliConnMap::iterator, bool> ret = m_conns.insert(CliConnMap::value_type(cid, conn));
		m_conns[cid] = conn;
		return conn;
	}
	int32 EventLoop::delConn(const std::string& cid)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "EventLoop::delConn cid=%s", cid.c_str())
		CliConnMap::iterator it = m_conns.find(cid);
		if (it != m_conns.end())
		{
			delete it->second;
			m_conns.erase(it);
			return 0;
		}
		return -1;
	}
}