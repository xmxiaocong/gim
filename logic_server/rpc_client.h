#ifndef __RPC_CLIENT_H__
#define __RPC_CLIENT_H__

#include "net/ef_sock.h"

namespace gim{

class ServiceRequest;
class ServiceResponse;

//typedef int (*ResponseCallBack)(void* par, const ServiceResponse& resp);

class RPCClient{
public:
	RPCClient();
	~RPCClient();
	int init(const std::string& ip, int port, int retrytime = 3);
	int callService(const ServiceRequest& req, ServiceResponse& resp, 
		int timeout_ms = 100);
	int disconnect();

private:
	int doRecv(int& cmd, std::string& msg, int timeout_ms = 100);
	int doSend(int cmd, const std::string& req, int timeout_ms = 100);
	int doCallService(const ServiceRequest& req, ServiceResponse& resp, 
		int timeout_ms = 100);

	int login();
	int reconnect();
	std::string m_server_ip;
	int m_server_port;
	ef::SOCKET m_sock;
	std::string m_sessid;
	std::string m_sn;
	int m_retrytime;
};

};


#endif/*__RPC_CLIENT_H__*/
