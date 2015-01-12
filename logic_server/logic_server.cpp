#include "logic_server.h"
#include "base/ef_utility.h"
#include "base/ef_tsd_ptr.h"

namespace gim{

TSDPtr<Dispatcher> g_Diss;

Dispatcher* LogicServer::getDispatcher(){	
	Dispatcher *pDBC = g_Diss.get();

	if (!pDBC) {
		pDBC = new Dispatcher();

		if(pDBC->init(m_ssch_conf) < 0){
			delete pDBC;
			return NULL;
		}

		g_Diss.set(pDBC);
	}

	return  pDBC;

}

LogicServer::LogicServer()
	:m_thread_cnt(0),
	m_keepalive_span(6000),
	m_reconnect_span(0),
	m_service_type(0),
	m_confac(NULL){
}


LogicServer::~LogicServer(){
}


int LogicServer::startListen(int port, ConnectionFactory* cfac, 
		ConnectionDispatcher* d){
	return m_cliset.startListen(port, cfac, d);
}

int LogicServer::stopListen(int port){
	return m_cliset.stopListen(port);
}

int LogicServer::initServerListCache(const Json::Value& v){
	SvLstChFactory slf;
	m_cache = slf.getSvLstCache(v);

	if(!m_cache){
		return -1;
	}

	m_cache->setServerListListener(this);
	m_cache->watchServerList(0);

	return 0;
}

//if has new connect server, connect it
int LogicServer::onListChange(int type, vector<Serv> &slist){

	for(size_t i = 0; i < slist.size(); ++i){
		size_t j = 0;

		//find if server is new
		for(; j < m_servlist.size(); ++j){
			if(slist[i].id == m_servlist[j].id)
				break;
		}

		if(j >= m_servlist.size()){
			allThreadConnectServer(slist[i]);
			m_servlist.push_back(slist[i]);
		}

	}		

	return 0;
}

int LogicServer::connectIPArray(SvCon* c, const Json::Value& a, 
	int port){

	int ret = 0;
	string addr;

	if(a.size() <= 0){
		return -1;
	}

	for(Json::UInt i = 0; i < a.size(); ++i){
		addr = a[i].asString();
		ret = c->connectTo(addr, port);
		
		if(ret >= 0){
			c->setAddr(addr, port);
			break;	
		}
	}

	return ret;

}

int LogicServer::allThreadConnectServer(const Serv& s){

	for(int i = 0; i < m_thread_cnt; ++i){
		connectServer(s);
	}
	
	return 0;
}

int LogicServer::connectServer(const Serv& s){
	int ret = 0;
	SvCon* c = m_confac->createSvCon(NULL);

	c->setConnectServerId(s.id);
	c->setServiceType(m_service_type);
	c->setReconnectSpan(m_reconnect_span);
	c->setKeepAliveSpan(m_keepalive_span);
	c->setLogicServer(this);
	
	int port = s.v[GIM_SERVER_LISTEN_PORT].asInt();

	//try localips
	ret = connectIPArray(c, s.v[GIM_LOCAL_IPS], port);
	
	//try publicips
	if(ret < 0){
		ret = connectIPArray(c, s.v[GIM_PUBLIC_IPS], port);
	}

	static volatile int id = 0;
	int conid = atomicIncrement32(&id);
	EventLoop& l = m_cliset.getEventLoop(conid % 
		m_cliset.getEventLoopCount());
	c->setId(conid);
	l.asynAddConnection(conid, c);	
	return 0;
}

int LogicServer::run(){
	m_cliset.run();
	return 0;	
}

int LogicServer::stop(){
	m_cliset.stop();
	return 0;
}

int LogicServer::init(int threadcnt, const Json::Value& svlstconf,
	const Json::Value& sschconf){
	m_thread_cnt = threadcnt;
	m_cliset.setEventLoopCount(threadcnt);
	m_cliset.init();
	initServerListCache(svlstconf);
	initSessCacheConfig(sschconf);
	return 0;
}

};
