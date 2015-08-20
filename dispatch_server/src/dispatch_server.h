#ifndef __DISPATCH_SERVER_H__
#define __DISPATCH_SERVER_H__

#include <map>
#include <vector>
#include <string>
#include "net/ef_server.h"
#include "json/json.h"
#include "config.h"
#include "zk_client.h"
#include "zk_server_ob.h"
#include "zk_config.h"

namespace ef {
	class EventLoop;
};

namespace gim{

class CliConFactory;
class CDispatcher;
class ZKClient;
class ZKConfig;
class ZKServerNode;

struct ConsvCfg {
	std::vector<std::string> iplist;
	int port;
};

typedef std::vector<ConsvCfg> ConsvLst;

struct typemap {
	int mintype;
	int maxtype;
	ConsvLst svlst;
};

class DispatchServer{
public:
	DispatchServer():m_run(false),m_zkc(NULL),m_zkcnf(NULL),m_zksvob(NULL){};
	int init(const std::string &confPath);
	int start();
	int stop();
	int getConsv(int type, const std::string &hashseed, ConsvCfg &csvcfg);
	ef::EventLoop& getEventLoop(int idx);

	const DPConfig& getConfig() const;

private:
	int loadConsvCfg();
	int initLog();
	int startListen();
	int stopListen();
	int loadConfig(const Json::Value& v);
	int loadConsvCfg(const children_map &children);

	static void ConfUpdateCallback(void* ctx, int evtype, const Json::Value& notify);
	static int ConsvObCallBack(void* ctx, const children_map& children);

	DPConfig m_conf;
	ef::Server m_serv;
	std::vector<typemap> m_tms;
	CliConFactory *m_cfac;
	CDispatcher *m_cdisp;

	bool m_run;

	ZKClient* m_zkc;
	ZKConfig* m_zkcnf;
	ZKServerListOb *m_zksvob;
};

};


#endif/*__DISPATCH_SERVER_H__*/
