#ifndef __SERVER_CONN_H__
#define __SERVER_CONN_H__

#include "net/ef_client.h"
#include "base/ef_atomic.h"
#include "connect_server.pb.h"
#include "logic_common.h"
#include "sess_cache.h"
#include <string>


namespace gim{


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
		m_sv_type(0), m_serv(NULL){
	}

	virtual	~SvCon();

	void setConnectServerId(int consvid){
		m_con_serv_id = consvid;	
	}

	int getConnectServerId() const{
		return m_con_serv_id;
	}

	void setSvType(int type){
		m_sv_type = type;
	}

	int getSvType() const{
		return m_sv_type;
	}

	void setLogicServerId(int id){
		m_svid = id;
	}

	int getLogicServerId() const{
		return m_svid;
	}

	void setLogicServer(LogicServer* s){
		m_serv = s;
	}
	
	LogicServer* getLogicServer(){
		return m_serv;
	}

	const std::string& getSessID() const{
		return m_sessid;
	}

	virtual int onCreate(ef::EventLoop* l);
	virtual int onConnected();
	virtual int onDisconnected();
	virtual int keepAlive();
	virtual int handlePacket(const std::string& req);

	// < 0, error, = 0, wait more, > 0, recv whole pack
	virtual int checkPackLen();

	virtual int handleServiceRequest(const ServiceRequest& req) = 0;
	//maybe from other logic server
	virtual int handleServiceResponse(const ServiceResponse& resp) = 0;

	int sendServiceResponse(const ServiceResponse& resp);
	int sendServiceRequest(const ServiceRequest& req);

	int callService(const ServiceRequest& req, ServiceResponse& resp, 
		int timeout_ms = 100);

	//int sendServiceRequest(int svtype, const std::string& sn,
	//	const std::string& key,
	//	const std::string& payload, const std::string& callback = "");

	int sendToClient(const std::string& cid, const ServiceRequest& req);
	int sendToClient(const std::string& cid, const ServiceResponse& resp);

	int doRegister();
	virtual int processRegisterResponse(const char* respbody, int len);
	virtual int processServiceRequest(const char* respbody, int len);
	virtual int processServiceResponse(const char* respbody, int len);

protected:

private:
	Dispatcher* getDispatcher();

	int m_status;
	int m_con_serv_id;
	int m_svid;
	int m_sv_type;
	std::string m_sessid;
	LogicServer* m_serv;
};

class SvConFactory{
public:
	virtual SvCon* createSvCon(void* par) = 0;
};

};//end of namespace ef

#endif
