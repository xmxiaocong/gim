#ifndef SERVER_CONN_H
#define SERVER_CONN_H

#include "net/ef_client.h"
#include "base/ef_atomic.h"
#include "logic_common.h"
#include "sess_cache.h"
#include <string>


namespace gim{

using namespace ef;

class LogicServer;
class Dispatcher;

class SvCon:public Client
{
public:
	enum{
		STATUS_INIT = 0,
		STATUS_LOGIN = 1,
		CHECK_TIMER = 1,
	};

	SvCon():m_status(STATUS_INIT),
		m_service_type(0), m_serv(NULL){
	}

	virtual	~SvCon(){
	}

	void setConnectServerId(int consvid){
		m_con_serv_id = consvid;	
	}

	int getConnectServerId() const{
		return m_con_serv_id;
	}

	void setServiceType(int type){
		m_service_type = type;
	}

	int getServiceType() const{
		return m_service_type;
	}

	void setServerId(int id){
		m_svid = id;
	}

	int getServerId() const{
		return m_svid;
	}

	void setLogicServer(LogicServer* s){
		m_serv = s;
	}
	
	LogicServer* getLogicServer(){
		return m_serv;
	}

	Dispatcher* getDispatcher();

	virtual int onConnected();
	virtual int onDisconnected();
	virtual int keepAlive();
	virtual int handlePacket(const std::string& req);

	// < 0, error, = 0, wait more, > 0, recv whole pack
	virtual int checkPackLen();

	virtual int handleServiceRequest(const char* reqbody, 
		int len, std::string& respbody) = 0;
	virtual int handleServiceResponse(const char* respbody, 
		int len) = 0;
protected:

private:
	int handleRegisterResponse(const char* respbody, int len);
	int doRegister();

	int m_status;
	int m_con_serv_id;
	int m_svid;
	int m_service_type;
	std::string m_sessid;
	LogicServer* m_serv;
};

class SvConFactory{
public:
	virtual SvCon* createSvCon(void* par) = 0;
};

};//end of namespace ef

#endif
