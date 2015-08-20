#include "connect_server.h"
#include <fstream>
#include "log_init.h"
#include "client_conn.h"
#include "zk_client.h"
#include "zk_config.h"
#include "zk_server_node.h"
#include "sess_cache.h"
#include "type_map.h"
#include "net/ef_server.h"
#include "net/ef_net_log.h"
#include "base/ef_statistic.h"
#include "base/ef_utility.h"
#include "dynamic_tokenchk.h"


namespace gim{

using namespace std;
using namespace ef;

static int output_statistic(void* par, const string& l){
	ConnectServer* s = (ConnectServer*)par;
	logError("ConnectStatistic") << "<svid:" << s->getConfig().ID
		<< "> " << l;
	return 0;
}


static void output_zklog(void* par, const string& l){
	ConnectServer* s = (ConnectServer*)par;
	logError("ConnectZookeeper") << "<svid:" << s->getConfig().ID
		<< "> " << l;
}



int ConnectServer::initLog(){
	logInit(m_conf.LogConfig);
	initStatistic(output_statistic, this);

	return 0;
}



int ConnectServer::initCompent(){
	int ret = 0;
	TypeMap::init(m_conf.MaxType);
	ret = SsChFactory::initSsChFactory(m_zkc, m_conf.SessCacheConfig);
	
	m_tkck = new DynamicTokenChk();
				
	if(!m_tkck){
		return -1;
	}

	m_tkck->setTimeout(3600 * 24 * 14);

	ret = ((DynamicTokenChk*)m_tkck)->init(m_zkc, m_conf.TokenKeyPath);
	
	if(ret < 0){
		return ret;
	}

	return ret;
}

int ConnectServer::freeCompent(){
	SsChFactory::freeSsChFactory();

	if(m_zksvnd){
		delete m_zksvnd;
		m_zksvnd = NULL;
	}

	if(m_zkcnf){
		delete m_zkcnf;
		m_zkcnf = NULL;
	}

	if(m_zkc){
		delete m_zkc;
		m_zkc = NULL;
	}

	if(m_tkck){
		delete m_tkck;
		m_tkck = NULL;
	}

	return 0;
}

int ConnectServer::startListen(){	
	int ret = 0;

	setNetLogName(m_conf.NetLogName);	
	m_serv.setEventLoopCount(m_conf.ThreadCount);
	ret = m_serv.init();

	if(ret < 0){
		return ret;
	}


	std::map<int, CliConfig>::iterator itor = m_conf.CliConfigs.begin();

	for(; itor != m_conf.CliConfigs.end(); ++itor){

		CliConfig& cliconf = itor->second;

		CliConFactory* cfac = new CliConFactory(this, &cliconf);
		CDispatcher* cdisp = new CDispatcher(this, &cliconf);
		ret = m_serv.startListen(cliconf.ListenPort, cfac, cdisp);
		if(ret < 0){
			cout << "listen at port:" << cliconf.ListenPort << " fail!\n";
			return ret;
		}
		m_cfacs.push_back(cfac);
		m_cdisps.push_back(cdisp);
	}

	return 0;
}

int ConnectServer::stopListen(){	
	int ret = 0;


	std::map<int, CliConfig>::iterator itor = m_conf.CliConfigs.begin();

	for(; itor != m_conf.CliConfigs.end(); ++itor){

		CliConfig& cliconf = itor->second;

		ret = m_serv.stopListen(cliconf.ListenPort);
		if(ret < 0){
			cout << "stop listen port:" << cliconf.ListenPort << " fail!\n";
			return ret;
		}
	}

	for(size_t i = 0; i < m_cfacs.size(); ++i){
		delete m_cfacs[i];
		delete m_cdisps[i];
	}

	return 0;
}

int ConnectServer::start(){
	m_serv.run();
	m_run = true;
	while(m_run){
		reportStatus();
		sleep_ms(10000);
	}
	return 0;
}

int ConnectServer::stop(){
	m_run = false;
	stopListen();
	m_serv.stop();
	return 0;
}

int ConnectServer::init(const string& zkurl, const string& path, 
		const std::string& statuspath, int id, const string& logconf){
	int ret = 0;

	m_conf.LogConfig = logconf;	

	initLog();

	ret = initConfigByZK(zkurl, path, id);
	if(ret < 0){
		cout << "initConfigByZK fail!" << endl;
		return ret;
	}

	ret = initStatusNode(statuspath, id);
	if(ret < 0){
		cout << "initStatusNode fail!" << endl;
		return ret;
	}

	ret = initCompent();
	
	if(ret < 0){
		cout << "initCompent fail!" << endl;
		return ret;
	}

	ret = startListen();

	return ret;
	
}

int ConnectServer::free(){
	return freeCompent();
}

EventLoop& ConnectServer::getEventLoop(int idx){
	return m_serv.getEventLoop(idx);
}

const ServerConfig& ConnectServer::getConfig() const{
	return m_conf;
}


void ConnectServer::ConfUpdateCallback(void* ctx, int ver, const Json::Value& notify){
	ConnectServer* c = (ConnectServer*)ctx;

	c->loadConfig(notify);

}

int ConnectServer::initConfigByZK(const string& zkurl, 
	const string& path, int id){

        m_zkc = new ZKClient();
        if(m_zkc->init(zkurl) < 0){
                delete m_zkc;
                return -5;
        }	

	m_zkc->setLogFn(this, output_zklog);

	m_conf.ID = id;	
	m_zkcnf = new ZKConfig(m_zkc, path, itostr(id));

	if(!m_zkcnf){
		return -10;
	}

	int ret = m_zkcnf->init(ConfUpdateCallback, this);	

	ret = m_zkcnf->getCfg(m_jsonconf);
	
	if(ret < 0){
		return -11;
	}

	ret = loadConfig(m_jsonconf);

	return ret;
}



static int getPublicIPFromShell(std::string& ip){
	FILE* f = NULL;
	char buf[1024] = {0};

	f = popen("curl ipinfo.io", "r");

	if(!f){
		return -1;
	}

	fread(buf, sizeof(buf), 1, f);
	pclose(f);

	Json::Reader reader;
	Json::Value root;
	if(reader.parse(buf, root)){
		ip = root["ip"].asString();
	}else{
		return -2;
	}

	return 0;
}

int ConnectServer::initStatusNode(const std::string& statuspath, int id){

	m_status = m_jsonconf;

	m_zksvnd = new ZKServerNode(m_zkc, statuspath, itostr(id));

	int ret = 0;

	string pubip;
	
	ret = getPublicIPFromShell(pubip);

	if(ret < 0){
		return -13;
	}

	vector<string> ips;
	getIPs(ips);


	bool ad = true;

	for(size_t i = 0; i < ips.size(); ++i){
		m_status["IPs"].append(ips[i]);
		if(ips[i] == pubip){
			ad = false;
		}
	}

	if(ad){
		m_status["IPs"].append(pubip);
	}

	ret = m_zksvnd->init(m_status.toStyledString());
	
	if(ret < 0){
		return -12;
	}

	return 0;
}



int ConnectServer::loadConfig(const Json::Value& root){
			
	const Json::Value& ThreadCountV = root["ThreadCount"];

	if(!ThreadCountV.isInt()){
		cout << "get ThreadCount fail!\n";
		return -1;
	}

	m_conf.ThreadCount = ThreadCountV.asInt();

	const Json::Value& NetLogNameV = root["NetLogName"];

	m_conf.NetLogName = NetLogNameV.asString();

	if(!NetLogNameV.isString()){
		cout << "get NetLogName fail!\n";
		return -2;
	}

	const Json::Value& MaxTypev = root["MaxType"];
	
	if(!MaxTypev.isInt()){
		cout << "get MaxType fail!\n";
		return -4;
	}


	m_conf.SessCacheConfig = root["SessCacheConfig"];

	m_conf.MaxType = MaxTypev.asInt();


	const Json::Value& CliConfigsV = root["CliConfigs"];

	if(!CliConfigsV.isArray()){
		cout << "get CliConfigs fail!\n";
		return -3;
	}

	const Json::Value& TokenKeyPathv = root["TokenKeyPath"];

	if(!TokenKeyPathv.isString()){
		cout << "get TokenKeyPath fail!\n";
		return -4;
	}

	m_conf.TokenKeyPath = TokenKeyPathv.asString();


	int ret = loadCliConfigs(CliConfigsV);

	return ret;	
}

int ConnectServer::reportStatus(){
	if(!m_zksvnd){
		return 0;
	}
	m_status["ConnectionCount"] = CliCon::connectionCount();
	m_status["ReportTimestamp"] = (int)time(NULL);


	int ret = m_zksvnd->setJson(m_status); 

	if(ret < 0){
		logError("ConnectZookeeper") << "<svid:" << getConfig().ID
			<< "> <action:reportStatus> <status:" << ret << ">";
	}

	return ret;
}


	
int ConnectServer::loadCliConfigs(const Json::Value& v){


	for(Json::Value::const_iterator itr = v.begin(); 
		itr != v.end(); ++itr){

		const Json::Value& cf = *itr;	
		
		const Json::Value& Encv = cf["Enc"];

		if(!Encv.isInt()){
			return -1;
		}
		
		const Json::Value& AliveMsv = cf["AliveMs"];
		if(!AliveMsv.isInt()){
			return -1;
		}

		const Json::Value& MinTypev = cf["MinType"];		
		if(!MinTypev.isInt()){
			return -1;
		}

		const Json::Value& MaxTypev = cf["MaxType"];
		if(!MaxTypev.isInt()){
			return -1;
		}
		
		const Json::Value& ListenPortv = cf["ListenPort"];
		if(!ListenPortv.isInt()){
			return -1;
		}

		const Json::Value& MaxReqQueSizev = cf["MaxReqQueSize"];
		if(!MaxReqQueSizev.isInt()){
			return -1;
		}

		const Json::Value& MaxPackCntPerMinv = cf["MaxPackCntPerMin"];
		if(!MaxPackCntPerMinv.isInt()){
			return -1;
		}

		const Json::Value& StartThreadIdxv = cf["StartThreadIdx"];
		if(!StartThreadIdxv.isInt()){
			return -1;
		}

		const Json::Value& ThreadCntv = cf["ThreadCnt"];		
		if(!ThreadCntv.isInt()){
			return -1;
		}

		CliConfig c;
		c.Enc = Encv.asInt();
		c.AliveMs = AliveMsv.asInt();
		c.MinType = MinTypev.asInt();
		c.MaxType = MaxTypev.asInt();
		c.ListenPort = ListenPortv.asInt();
		c.MaxReqQueSize = MaxReqQueSizev.asInt();
		c.MaxPackCntPerMin = MaxPackCntPerMinv.asInt();
		c.StartThreadIdx = StartThreadIdxv.asInt();
		c.ThreadCnt = ThreadCntv.asInt();
	
		m_conf.CliConfigs[c.ListenPort] = c;	
			
	}

	return 0;
}

};
