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


	virtual int onConnected();
	virtual int onDisconnected();
	virtual int keepAlive();
	virtual int handlePacket(const std::string& req);

	// < 0, error, = 0, wait more, > 0, recv whole pack
	virtual int checkPackLen();

	virtual int handleServiceRequest(const std::string& payload) = 0;
	//maybe from other logic server
	virtual int handleServiceResponse(int svtype,
		int status, const std::string& payload,
		const std::string& callback) = 0;

	int sendServiceResponse(int status, 
		const std::string& payload);
	int sendServiceRequest(int svtype, const std::string& key,
		const std::string& payload, const std::string& callback = "");

	int sendServiceRequestToClient(const string& cid, const std::string& sn, 
		const string& payload, const std::string& callback = "");
	int sendServiceResponseToClient(const string& cid, const std::string& sn,
		int status, const string& payload, 
		const std::string& callback = "");

	std::string getPackFromSessid() const{
		return m_from_sessid;
	}

	std::string getPackSN() const{
		return m_cur_pack_sn;
	}

	std::string getPackKey() const{
		return m_cur_pack_key;
	}

	std::string getPackCallBack() const{
		return m_callback;
	}

	int getPackSvType() const{
		return m_packsvtype;
	}

	int processRegisterResponse(const char* respbody, int len);
	int doRegister();
	int processServiceRequest(const char* respbody, int len);
	int processServiceResponse(const char* respbody, int len);

protected:

private:

	Dispatcher* getDispatcher();

	int m_status;
	int m_con_serv_id;
	int m_svid;
	int m_service_type;
	std::string m_sessid;
	std::string m_from_sessid;
	std::string m_cur_pack_sn;
	std::string m_cur_pack_key;
	std::string m_callback;
	int m_packsvtype;
	LogicServer* m_serv;
};

class SvConFactory{
public:
	virtual SvCon* createSvCon(void* par) = 0;
};

};//end of namespace ef

#endif
