#ifndef __LOGIC_SERVER_H__
#define __LOGIC_SERVER_H__


#include <map>
#include "server_conn.h"
#include "server_status.h"
#include "json/json.h"
#include "net/ef_server.h"
#include "base/ef_loader.h"

namespace gim{

typedef int (*ServerObjInit)(void* obj, LogicServer* s);
typedef int (*ServerObjClean)(void* obj, LogicServer* s);

class ZKClient;
class ZKConfig;
class ZKServerNode;
class ZKServerListOb;

struct LogicConfig{
	int ID;
	int Type;
	int ThreadCount;
	int KeepaliveSpanMS;
	int ReconnectSpanMS;

	std::string LogConfig;
	std::string NetLogName;

	std::string LogName;

	Json::Value SessCacheConfig;


	LogicConfig():ThreadCount(12), KeepaliveSpanMS(30000), 
		ReconnectSpanMS(10000){
	}
};

class LogicServer: public ef::Server{
public:
	LogicServer();
	~LogicServer();


	void setSvConFactory(SvConFactory* f){
		m_confac = f;
	}

	SvConFactory* getSvConFactory(){
		return m_confac;
	}

	int init(const std::string& zkurl, const std::string& confpath,
		const std::string& connectstatuspath,
		const std::string& statuspath, int type, int id, 
		const std::string& logconf);

	int setObj(void* obj, ServerObjInit i, ServerObjClean c){
		if(m_obclean){
			m_obclean(m_obj, this);
		}
		m_obj = obj;
		m_obinit = i;
		m_obclean = c;
		return 0;
	}


	int start();
	int stop();

	const LogicConfig& getConfig() const{
		return m_conf;
	}

	ef::DataSrc<Json::Value>& getJsonConfig(){
		return m_jsonconf;
	}

	int free();
	int reportStatus();
	
private:

	typedef std::map<int, ServerStatus> SMap;

	int initConfig(const std::string& zkurl, const std::string& confpath,
		int type, int id);
	int initStatusNode(const std::string& statuspath);
	int initSvListOb(const std::string& connectstatuspath);
	int initEventLoop();
	int initLog();
	int initCompent();
	int freeCompent();
	int loadConfig(const Json::Value& conf);
	int notifySvlistChange();

	int initDispatcher();

	static void ConfUpdateCallback(void* ctx, int ver, const Json::Value& notify);
	static int ServerListChangeCallback(void* ctx, 
		const std::map<std::string, Json::Value>& notify);

	LogicConfig m_conf;
	ef::Server m_serv;

	ef::DataSrc<Json::Value> m_jsonconf;
	Json::Value m_status;

	bool m_run;

	ZKClient* m_zkc;
	ZKConfig* m_zkcnf;
	ZKServerListOb* m_zksvob;
	ZKServerNode* m_zksvnd;
	SvConFactory* m_confac;
	ef::DataSrc<SMap> m_svlst;
	void* m_obj;
	ServerObjInit m_obinit;
	ServerObjClean m_obclean;
};


};

#endif
