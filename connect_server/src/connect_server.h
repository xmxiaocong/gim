#ifndef __CONNECT_SERVER_H__
#define __CONNECT_SERVER_H__

#include <map>
#include <vector>
#include <string>
#include "client_config.h"
#include "net/ef_server.h"
#include "json/json.h"

namespace ef{
class EventLoop;
};

namespace gim{

class CliConFactory;
class CDispatcher;
class ZKClient;
class ZKConfig;
class ZKServerNode;
class TokenChecker;


struct ServerConfig{
	int ID;
	int ThreadCount;
	int MaxType;

	std::map<int, CliConfig> CliConfigs;

	std::string LogConfig;
	std::string NetLogName;

	Json::Value SessCacheConfig;

	std::string TokenKeyPath;	
};


class ConnectServer{
public:

	ConnectServer():m_run(false), m_zkc(NULL), m_zkcnf(NULL),
		m_zksvnd(NULL){
		}

	int init(const std::string& zkurl, const std::string& path,
		const std::string& statuspath, int id, const std::string& logconf);
	int free();
	int start();
	int stop();
	ef::EventLoop& getEventLoop(int idx);

	const ServerConfig& getConfig() const;


	TokenChecker* getTokenChecker(){
		return m_tkck;
	}

private:
	int initConfigByZK(const std::string& zkurl, const std::string& path, int id);
	int initStatusNode(const std::string& statuspath, int id);
	int initLog();
	int reportStatus();
	int initCompent();
	int freeCompent();
	int startListen();
	int stopListen();
	int loadConfig(const Json::Value& v);
	int loadCliConfigs(const Json::Value& v);

	static void ConfUpdateCallback(void* ctx, int ver, const Json::Value& notify);


	ServerConfig m_conf;
	Json::Value m_jsonconf;
	ef::Server m_serv;

	std::vector<CliConFactory*> m_cfacs;
	std::vector<CDispatcher*> m_cdisps;

	Json::Value m_status;


	bool m_run;

	ZKClient* m_zkc;
	ZKConfig* m_zkcnf;
	ZKServerNode* m_zksvnd;
	TokenChecker* m_tkck;

};

};



#endif/*__CONNECT_SERVER_H__*/
