#include <fstream>
#include "log_init.h"
#include "zk_client.h"
#include "zk_config.h"
#include "zk_server_ob.h"
#include "net/ef_server.h"
#include "net/ef_net_log.h"
#include "base/ef_statistic.h"
#include "base/ef_utility.h"
#include "base/ef_md5.h"
#include "dispatch_server.h"
#include "server_status.h"
#include "client_conn.h"
#include <iostream>

namespace gim{

using namespace std;
using namespace ef;

static int output_statistic(void* par, const string& l)
{
	DispatchServer* s = (DispatchServer*)par;
	logError("ConnectStatistic") << "<svid:" << s->getConfig().ID
		<< "> " << l;
	return 0;
}

int DispatchServer::initLog()
{
	logInit(m_conf.LogConfig);
	initStatistic(output_statistic, this);

	return 0;
}

int DispatchServer::loadConsvCfg(const children_map& children)
{
	vector<typemap> tms;
	std::map<string, Json::Value>::const_iterator it = children.begin();
	for (; it != children.end(); it++) {
		if (!it->second.isObject()) {
			return -1;
		}
		ServerStatus ss;
		ss.parseFromJson(it->second);
		vector<PortConfig>::iterator pit = ss.Ports.begin();
		for (; pit != ss.Ports.end(); pit++) {
			bool exist = false;
			vector<typemap>::iterator tit = tms.begin();
			for (; tit != tms.end(); tit++) {
				if (pit->MinType == tit->mintype || pit->MaxType == tit->maxtype) {
					exist = true;
					ConsvCfg csvcfg;
					csvcfg.port = pit->Port;
					csvcfg.iplist = ss.IPs;
					tit->svlst.push_back(csvcfg);
				}
			}
			if (!exist) {
				typemap tm;
				tm.mintype = pit->MinType;
				tm.maxtype = pit->MaxType;
				ConsvCfg csvcfg;
				csvcfg.port = pit->Port;
				csvcfg.iplist = ss.IPs;
				tm.svlst.push_back(csvcfg);
				tms.push_back(tm);
			}
		}
	}
	
	if (tms.size()) m_tms = tms;
	return 0;
}

int DispatchServer::init(const std::string &confPath)
{
	fstream f;
	int ret = 0;

	f.open(confPath.data(), ios::in);

	if(!f.is_open()){
		cout << "<config:" << confPath << "> open config file fail\n";
		return -1;
	}

	Json::Reader reader;
	Json::Value root;

	if(!reader.parse(f, root, false)){
		cout << "<config:" << confPath << "> parse config file fail\n";
		return -1;
	}

	m_conf.ID = root["ID"].asInt();
	m_conf.ThreadCnt = root["ThreadCnt"].asInt();
	m_conf.Daemon = root["Daemon"].asInt();
	m_conf.ListenPort = root["ListenPort"].asInt();
	m_conf.StartThreadIdx = root["StartThreadIdx"].asInt();
	m_conf.ListenIP = root["ListenIp"].asString();
	m_conf.zkurl = root["zkurl"].asString();
	m_conf.ConsvPath = root["ConsvPath"].asString();
	m_conf.LogConfig = root["LogConfig"].asString();
	m_conf.NetLogName = root["NetLogName"].asString();

	f.close();
	initLog();

	children_map cmap;
	m_zkc = new ZKClient();
	m_zkc->init(m_conf.zkurl);
	m_zksvob = new ZKServerListOb(m_zkc, m_conf.ConsvPath);
	m_zksvob->init(true, ConsvObCallBack, this);
	m_zksvob->getServerList(cmap);

	loadConsvCfg(cmap);
	ret = startListen();

	return ret;
}

int DispatchServer::getConsv(int type, const string &hashseed, ConsvCfg &csvcfg)
{
	uint8 md5bytes[16];
	vector<typemap>::iterator it = m_tms.begin();
	for (; it != m_tms.end(); it++) {
		if (it->mintype <= type && it->maxtype >= type) {
			ef::MD5(md5bytes, (const uint8*)hashseed.data(), hashseed.size());
			int i = *(int32*)md5bytes;
			int idx = i % it->svlst.size();
			csvcfg = it->svlst[idx];
			return 0;
		}
	}
	return -1;
}

int DispatchServer::startListen()
{	
	int ret = 0;

	m_serv.setEventLoopCount(m_conf.ThreadCnt);
	ret = m_serv.init();

	if(ret < 0){
		return ret;
	}

	m_cfac = new CliConFactory(this, &m_conf);
	m_cdisp = new CDispatcher(this, &m_conf);
	ret = m_serv.startListen(m_conf.ListenPort, m_cfac, m_cdisp);
	if(ret < 0){
		return ret;
	}

	return 0;
}

int DispatchServer::stopListen(){	
	int ret = 0;

	ret = m_serv.stopListen(m_conf.ListenPort);
	if(ret < 0){
		cout << "stop listen port:" << m_conf.ListenPort << " fail!\n";
		return ret;
	}

	delete m_cfac;
	delete m_cdisp;

	return 0;
}

int DispatchServer::start(){
	m_serv.run();
	m_run = true;
	while(m_run){
		sleep_ms(1000);
	}
	return 0;
}

int DispatchServer::stop(){
	m_run = false;
	stopListen();
	if (m_zkcnf) delete m_zkc;
	if (m_zkcnf) delete m_zkcnf;
	if (m_zksvob) delete m_zksvob;

	return 0;
}

ef::EventLoop& DispatchServer::getEventLoop(int idx){
	return m_serv.getEventLoop(idx);
}

const DPConfig& DispatchServer::getConfig() const{
	return m_conf;
}


void DispatchServer::ConfUpdateCallback(void* ctx, 
	int evtype, const Json::Value& notify){
	DispatchServer* c = (DispatchServer*)ctx;
	c->loadConfig(notify);
}

int DispatchServer::ConsvObCallBack(void* ctx, const children_map& children)
{
	DispatchServer* c = (DispatchServer*)ctx;
	c->loadConsvCfg(children);
	return 0;
}

int DispatchServer::loadConfig(const Json::Value& root)
{
	return 0;
}

};
