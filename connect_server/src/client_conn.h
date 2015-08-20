#ifndef __CLIENT_CONN_H__
#define __CLIENT_CONN_H__

#include "base/ef_atomic.h"
#include "net/ef_connection.h"
#include "net/ef_operator.h"
#include "net/ef_acceptor.h"
#include "client_config.h"
#include "sess_cache.h"
#include <string>


namespace ef{
	class EventLoop;
};

namespace gim{

class head;
class ConnectServer;

class CliCon:public ef::Connection 
{
public:
	enum{
		STATUS_INIT = 0,
		STATUS_LOGIN = 1,
		CHECK_TIMER = 1,
	};

	CliCon(ConnectServer* s, CliConfig* conf)
		:m_serv(s), m_conf(conf),
		m_status(STATUS_INIT), 
		m_busy(0){
		ef::atomicIncrement32(&cnt);
	}

	virtual	~CliCon();

	virtual int onCreate(ef::EventLoop* l);

	virtual int handleTimer(ef::EventLoop* l, int id);
	virtual int handlePacket(const std::string& req);

	// < 0, error, = 0, wait more, > 0, recv whole pack
	virtual int checkPackLen();

	const std::string& sessId() const{
		return m_sess.sessid();
	}

	static int totalCount(){
		return cnt;
	}

	virtual int sendCmd(int cmd, const std::string& body);

	const CliConfig* getConfig() const{
		return m_conf;
	}

	int type() const{
		return m_sess.type();
	}

	static int connectionCount() {
		return cnt;
	}
	
protected:

private:
	int handleLoginRequest(const head& h,
			const std::string& req, std::string& resp);
	int handleServiceRequest(const head& h,
			const std::string& req, std::string& resp);
	int handleServiceResponse(const head& h,
			const std::string& req);
	int getConFromSessid(const std::string& sessid, 
		ef::EventLoop*& svlp, int& conid, std::string& old);

	int delSession();
	int updateSession();

	int decodeBody(const char* encbody, int bodylen, std::string& body);
	int encodeBody(const char* body, int bodylen, std::string& encbody);
	
	int checkType(int type);
	int checkToken(const std::string& tk);
	int checkReqQue(int reqcnt, int respcnt);

	int kickClient(const std::string& reason);

	ConnectServer* m_serv;
	CliConfig* m_conf;
	int m_status;
	time_t m_login_time;
	int m_busy;
	Sess m_sess;
	std::string m_key;

	ef::int64 m_req_cnt;
	ef::int64 m_resp_cnt;
	
	static volatile int cnt;
};

class SendMsgOp: public ef::NetOperator{
public:
	SendMsgOp(int conid, int cmd, const std::string& body)
		: m_conid(conid), m_cmd(cmd), m_body(body){
	}

	virtual int process(ef::EventLoop *l);

private:
	int m_conid;
	int m_cmd;
	std::string m_body;
};


class   CliConFactory:public ef::ConnectionFactory{
public:
	CliConFactory(ConnectServer* s, CliConfig* conf)
		:m_serv(s), m_conf(conf) 
		{
	}

	~CliConFactory(){

	}

	virtual ef::Connection* createConnection(ef::EventLoop* l,
			int fd, const std::string& addr, int port);
	
private:
	ConnectServer* m_serv;
	CliConfig* m_conf;
};


class	CDispatcher:public ef::ConnectionDispatcher{
public:
	CDispatcher(ConnectServer* s, CliConfig* conf)
		:m_s(s), m_conf(conf){}
	int dispatchConnection(ef::EventLoop*l, ef::Connection* c);
private:
	ConnectServer* m_s;
	CliConfig* m_conf;
};

};//end of namespace ef

#endif
