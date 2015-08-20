#include "logic_server.h"
#include "log_init.h"
#include "base/ef_utility.h"
#include "base/ef_statistic.h"
#include "dispatcher.h"
#include "zk_client.h"
#include "zk_config.h"
#include "zk_server_node.h"
#include "zk_server_ob.h"
#include "net/ef_operator.h"
#include "net/ef_net_log.h"

namespace gim{

using namespace std;
using namespace ef;

static int output_statistic(void* par, const string& l){
	LogicServer* s = (LogicServer*)par;
	logError("LogicStatistic") << "<svid:" << s->getConfig().ID
		<< "> " << l;
	return 0;
}



int LogicServer::initLog(){
	logInit(m_conf.LogConfig);
	initStatistic(output_statistic, this);

	return 0;
}

class CheckConnectServerOp: public NetOperator{
public:
	virtual int32 process(EventLoop *l){
		Dispatcher* d = (Dispatcher*)l->getObj();
		return d->checkConnectServers();
	}
private:
};

static int initDispatcherFn(EventLoop* l, void* obj){
	return 0;
}

static int deleteDispatcherFn(EventLoop* l, void* obj){
	delete (Dispatcher*)obj;
	return 0;
}

int LogicServer::initDispatcher(){	
	
	for(int i = 0; i< m_conf.ThreadCount; ++i){

		EventLoop& l = getEventLoop(i);
		Dispatcher* pDBC = new Dispatcher(this, &l, &m_svlst);

		if(pDBC->init(NULL) < 0){
			delete pDBC;
			return -1;
		}
		l.setObj(pDBC, initDispatcherFn, deleteDispatcherFn);
	}


	return  0;

}


int LogicServer::notifySvlistChange(){	
	
	for(int i = 0; i< m_conf.ThreadCount; ++i){

		EventLoop& l = getEventLoop(i);
		CheckConnectServerOp* op = new CheckConnectServerOp();
		l.addAsynOperator(op);
	}


	return  0;

}




LogicServer::LogicServer()
	:m_zkc(NULL), m_zkcnf(NULL), 
	m_zksvob(NULL), m_zksvnd(NULL),
	m_obj(NULL), m_obinit(NULL), m_obclean(NULL){
}


LogicServer::~LogicServer(){
	free();
}

int LogicServer::start(){
	setNetLogName(m_conf.NetLogName);
	return Server::run();
}

int LogicServer::stop(){
	Server::stop();
	return 0;
}

int LogicServer::init(const string& zkurl, const string& path, 
		const std::string& statuspath, const std::string& connectstatuspath,
		int type, int id, const string& logconf){
	int ret = 0;

	m_conf.LogConfig = logconf;	

	initLog();

	ret = initConfig(zkurl, path, type, id);
	if(ret < 0){
		return ret;
	}

	initEventLoop();

	ret = initStatusNode(statuspath);
	if(ret < 0){
		return ret;
	}

	ret = initSvListOb(connectstatuspath);
	if(ret < 0){
		return ret;
	}

	ret = initCompent();
	
	if(ret < 0){
		return ret;
	}

	initDispatcher();

	if(m_obinit){
		m_obinit(m_obj, this);
	}

	return ret;
	
}

int LogicServer::free(){

	if(m_obclean){
		m_obclean(m_obj, this);
	}

	freeCompent();
	if(m_zksvnd){
		delete m_zksvnd;
		m_zksvnd = NULL;
	}
	if(m_zksvob){
		delete m_zksvob;
		m_zksvob = NULL;
	}
	if(m_zkcnf){
		delete m_zkcnf;
		m_zkcnf = NULL;
	}
	if(m_zkc){
		delete m_zkc;
		m_zkc = NULL;
	}

	return 0;
}

int LogicServer::initCompent(){
	int ret = SsChFactory::initSsChFactory(m_zkc, m_conf.SessCacheConfig);
	return ret;
}

int LogicServer::freeCompent(){
        int ret = SsChFactory::freeSsChFactory();
        return ret;
}

int LogicServer::initConfig(const std::string& zkurl, 
	const std::string& confpath, int type, int id){

	m_conf.Type = type;
	m_conf.ID = id;


	m_zkc = new ZKClient();
	if(m_zkc->init(zkurl) < 0){
		delete m_zkc;
		return -5;
	}

	m_zkcnf = new ZKConfig(m_zkc, confpath, ef::itostr(id));

	if(!m_zkcnf){
		return -10;
	}

	int ret = m_zkcnf->init(ConfUpdateCallback, this);


	Json::Value jscnf;

	ret = m_zkcnf->getCfg(jscnf);

	if(ret < 0){
		return -11;
	}

	ret = loadConfig(jscnf);
	m_jsonconf.setData(jscnf);

	return ret;
}

int LogicServer::initEventLoop(){
	Server::setEventLoopCount(m_conf.ThreadCount);
	Server::init();

	return 0;
}

int LogicServer::reportStatus(){
	m_zksvnd->setData("iamhere");
	return 0;
}

void LogicServer::ConfUpdateCallback(void* ctx, int ver, const Json::Value& notify){
	LogicServer* s = (LogicServer*)ctx;
	s->m_jsonconf.setData(notify);
	s->loadConfig(notify);
//	return 0;
}

int LogicServer::ServerListChangeCallback(void* ctx, const children_map& notify){
	LogicServer* s = (LogicServer*)ctx;
	children_map::const_iterator itor = notify.begin();
	SMap m;
	std::cout << "\n\nServerListChangeCallback\n\n";
	for(; itor != notify.end(); ++itor){
		if(!itor->second.isObject()){
			std::cout << "\n\n+++:" << itor->second.toStyledString() << std::endl;
			continue;
		}
		Json::FastWriter w;
		std::cout << w.write(itor->second) << std::endl;	
		ServerStatus s2;
		ServerStatus s1;
		s2.ID = atoi(itor->first.data());
		s2.parseFromJson(itor->second);
		s1 = s2;
		s1.Ports.clear();
		size_t i = 0;
		for(; i < s2.Ports.size(); ++i){
			if(s2.Ports[i].MinType <= s->m_conf.Type 
				&& s2.Ports[i].MaxType > s->m_conf.Type){
				s1.Ports.push_back(s2.Ports[i]);
			}
		}
		if(!s1.Ports.size()){
			continue;
		}
		m[s1.ID] = s1;
	}
	s->m_svlst.setData(m);
	s->notifySvlistChange();
	return 0;
}


int LogicServer::initStatusNode(const std::string& statuspath){

	m_jsonconf.getData(m_status);

	m_zksvnd = new ZKServerNode(m_zkc, statuspath, 
		ef::itostr(m_conf.ID));

	int ret = m_zksvnd->init(ef::itostr(m_conf.ID));

	if(ret < 0){
		return -12;
	}

	m_zksvnd->setJson(m_status);

	return 0;
}

int LogicServer::initSvListOb(const std::string& connectstatuspath){

	m_zksvob = new ZKServerListOb(m_zkc, connectstatuspath);

	int ret = m_zksvob->init(true, ServerListChangeCallback, this);

	if(ret < 0){
		return -12;
	}

	return ret;	
}

int LogicServer::loadConfig(const Json::Value& root){

	const Json::Value& ThreadCountV = root["ThreadCount"];

	if(!ThreadCountV.isInt()){
		cout << "get ThreadCount fail!\n";
		return -1;
	}

	m_conf.ThreadCount = ThreadCountV.asInt();

	const Json::Value& NetLogNameV = root["NetLogName"];

	if(!NetLogNameV.isString()){
		cout << "get NetLogName fail!\n";
		return -2;
	}

	m_conf.NetLogName = NetLogNameV.asString();

	const Json::Value& LogNameV = root["LogName"];

	if(!LogNameV.isString()){
		cout << "get LogName fail!\n";
		return -2;
	}
	m_conf.LogName = LogNameV.asString();

	const Json::Value& KeepaliveSpanMSV = root["KeepaliveSpanMS"];

	if(!KeepaliveSpanMSV.isInt()){
		cout << "get KeepaliveSpanMS fail!\n";
		return -1;
	}

	m_conf.KeepaliveSpanMS = KeepaliveSpanMSV.asInt();

	const Json::Value& ReconnectSpanMSV = root["ReconnectSpanMS"];

	if(!ReconnectSpanMSV.isInt()){
		cout << "get ReconnectSpanMS fail!\n";
		return -1;
	}

	m_conf.ReconnectSpanMS = ReconnectSpanMSV.asInt();

	m_conf.SessCacheConfig = root["SessCacheConfig"];

	return 0;
}


};
