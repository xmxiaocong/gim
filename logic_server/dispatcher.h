#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__

#include <map>
#include <vector>
#include "sess_cache.h"
#include "base/ef_loader.h"
#include "server_status.h"

namespace ef{
	class EventLoop;
}

namespace gim{

typedef std::map<int, ServerStatus> StatusMap;
typedef ef::DataSrc<StatusMap> DtSrc;
typedef ef::Loader<StatusMap> Loader;

class SvCon;
class LogicServer;
class ServiceRequest;
class ServiceResponse;
class RPCClient;

enum{
	CID_OFFLINE = -20001,
	CONNECT_SERVER_OFFLINE = -20101,
	SEND_FAIL = -20201,	
};

class Dispatcher{
public:
	
	Dispatcher(LogicServer* s, ef::EventLoop* l, DtSrc* src);

	int init(void* par);

	~Dispatcher();

	int addConnectServer(int svid, SvCon* con);
	int delConnectServer(int svid);
	SvCon* getConnectServer(int svid);

	int sendToClient(const std::string& cid, const ServiceRequest& req); 
	int sendToClient(const std::string& cid, const ServiceResponse& resp); 
	int checkConnectServers();

	int callService(const ServiceRequest& req, ServiceResponse& resp, 
		int  timeout_ms = 100);
		
private:
	typedef std::pair<std::string, SvCon*> CliSess;
	int getClientLoginServer(const std::string& cid, std::vector<CliSess>& svs);
	int connectServer(const ServerStatus& s);
	int connectIPArray(SvCon* c, const std::vector<std::string>& a, int port);
	int reinitRPCClient(const StatusMap& m);
	int reinitRPCClient(const ServerStatus& s);

	Loader m_loader;
	std::map<int, SvCon*> m_svcons;
	LogicServer* m_sv;
	ef::EventLoop* m_evlp;
	SessCache* m_sesscache;
	RPCClient* m_rpc_cli;
};


};

#endif
