#ifndef __CLIENT_CONN_H__
#define __CLIENT_CONN_H__

#include "net/ef_server.h"
#include "net/ef_operator.h"
#include "base/ef_atomic.h"
#include "cache_conn.h"
#include "settings.h"
#include <string>


namespace gim{

using namespace ef;

class head;

class CliCon:public Connection
{
public:
	enum{
		STATUS_INIT = 0,
		STATUS_LOGIN = 1,
		CHECK_TIMER = 1,
	};

	CliCon(Server* s, int32 alive_ms)
		:m_serv(s), m_alive_ms(alive_ms),
		m_status(STATUS_INIT), 
		m_pack_cnt_one_cycle(0),
		m_last_check_time(0),
		m_enc(0){
		atomicIncrement32(&cnt);
	}

	virtual	~CliCon();

	virtual int32 onCreate(EventLoop* l);

	virtual int32 handleTimer(EventLoop* l, int32 id);
	virtual int32 handlePacket(const std::string& req);

	// < 0, error, = 0, wait more, > 0, recv whole pack
	virtual int32 checkPackLen();

	const std::string& sessId() const{
		return m_sess.sessid();
	}

	static int32 totalCount(){
		return cnt;
	}

	int32 sendToClient(int32 cmd, const std::string& body);
protected:

private:
	int32 handleLoginRequest(const head& h,
			const std::string& req, std::string& resp);
	int32 handleServiceRequest(const head& h,
			const std::string& req, std::string& resp);
	int32 handleServiceResponse(const head& h,
			const std::string& req);

	int32 kickClient(const std::string& reason);
	int32 setClientTime();
	int32 getServConFromSN(const std::string& sn,
			EventLoop*& l, int32& conid, std::string& oldsn);
	int32 delSession();
	int32 updateSession();

	std::string getSessKVsString();

	int32 frequencyControl();
	int32 checkTime(int32 max_dif);
	int32 checkToken(const std::string& token);
	int32 decodeBody(const char* encbody, int32 bodylen, std::string& body);
	int32 encodeBody(const char* body, int32 bodylen, std::string& encbody);


	Server* m_serv;
	int32 m_alive_ms;
	int32 m_status;
	int32 m_pack_cnt_one_cycle;
	time_t m_login_time;
	int32 m_last_check_time;
	int64 m_client_login_time;
	Sess m_sess;
	string m_version;
	int32 m_enc;
	std::string m_key;
	static volatile int32 cnt;
};

class	CliRespOp:public NetOperator{
public:
	CliRespOp(int32 conid, int32 cmd, const std::string& body):
		m_conid(conid), m_cmd(cmd), m_body(body){
	}

	virtual int32 process(EventLoop *l){
		CliCon* clic = (CliCon*)l->getConnection(m_conid);
		if(!clic){
			std::cout << "cli transMessage old id INVLID_SESSION_ID\n";
			return -1;
		}
		return clic->sendToClient(m_cmd, m_body);
	}
	
private:
	int32 m_conid;
	int32 m_cmd;
	std::string m_body;
};

class   CliConFactory:public ConnectionFactory{
public:
	CliConFactory(Server* s, int32 connectionAlivems)
		:m_serv(s), m_con_alive_ms(connectionAlivems), 
		m_cnt_one_cycle(0), m_check_time(0){
		m_check_time = time(NULL);
	}

	~CliConFactory(){

	}

	int32 acceptControl();

	virtual Connection* createConnection(EventLoop* l,
			int32 fd, const std::string& addr, int32 port);
	
private:
	Server* m_serv;
	int32 m_con_alive_ms;
	volatile int32 m_cnt_one_cycle;
	time_t m_check_time;
};


class	CDispatcher:public ConnectionDispatcher{
public:
	CDispatcher(Server* s):m_s(s){}
	int32 dispatchConnection(EventLoop*l, Connection* c);
private:
	Server* m_s;
};

};//end of namespace ef

#endif
