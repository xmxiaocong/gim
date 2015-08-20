#ifndef __CLIENT_CONN_H__
#define __CLIENT_CONN_H__

#include "base/ef_atomic.h"
#include "net/ef_connection.h"
#include "net/ef_operator.h"
#include "net/ef_acceptor.h"
#include "config.h"
#include <string>


namespace ef{
	class EventLoop;
};

namespace gim{

class head;
class DispatchServer;

class CliCon:public ef::Connection 
{
public:
	enum{
		STATUS_INIT = 0,
		STATUS_LOGIN = 1,
		CHECK_TIMER = 1,
	};

	CliCon(DispatchServer* s, DPConfig* conf)
		:m_serv(s), m_conf(conf),
		m_busy(0){
		ef::atomicIncrement32(&cnt);
	}

	virtual	~CliCon();

	virtual int onCreate(ef::EventLoop* l);

	virtual int handlePacket(const std::string& req);

	// < 0, error, = 0, wait more, > 0, recv whole pack
	virtual int checkPackLen();

	static int totalCount(){
		return cnt;
	}

	const DPConfig* getConfig() const{
		return m_conf;
	}
	
protected:

private:
	int handleLoginRequest(const head& h,
			const std::string& req, std::string& resp);

	DispatchServer* m_serv;
	DPConfig* m_conf;
	int m_busy;
	ef::int64 m_req_cnt;
	ef::int64 m_resp_cnt;
	
	static volatile int cnt;
};

class   CliConFactory:public ef::ConnectionFactory{
public:
	CliConFactory(DispatchServer* s, DPConfig* conf)
		:m_serv(s), m_conf(conf) 
		{
	}

	~CliConFactory(){

	}

	virtual ef::Connection* createConnection(ef::EventLoop* l,
			int fd, const std::string& addr, int port);
	
private:
	DispatchServer* m_serv;
	DPConfig* m_conf;
};


class	CDispatcher:public ef::ConnectionDispatcher{
public:
	CDispatcher(DispatchServer* s, DPConfig* conf)
		:m_s(s), m_conf(conf){}
	int dispatchConnection(ef::EventLoop*l, ef::Connection* c);
private:
	DispatchServer* m_s;
	DPConfig* m_conf;
};

};//end of namespace ef

#endif
