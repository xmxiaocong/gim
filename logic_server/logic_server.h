#ifndef __LOGIC_SERVER_H__
#define __LOGIC_SERVER_H__

#include "json/json.h"
#include "net/ef_client.h"
#include "net/ef_server.h"
#include "svlist_cache.h"
#include "server_conn.h"
#include "logic_conn.h"
#include "dispatcher.h"

namespace gim{

using namespace ef;

class LogicServer:public SvLstListener{
public:
	LogicServer();
	~LogicServer();

	void setKeepAliveSpan(int ms){
		m_keepalive_span = ms;
	}

	int keepAliveSpan() const{
		return m_keepalive_span;
	}

	void setReconnectSpan(int ms){
		m_reconnect_span = ms;
	}

	int reconnectSpan() const{
		return m_reconnect_span;
	}

	void setLogName(const string& name){
		m_logname = name;
	}

	string getLogName() const{
		return m_logname;
	}

	void setId(int id){
		m_id = id;
	}

	int getId() const{
		return m_id;
	}

	void setServiceType(int type){
		m_service_type = type;
	}

	int serviceType() const{
		return m_service_type;
	}

	void setSvConFactory(SvConFactory* f){
		m_confac = f;
	}

	int init(int threadcnt, const Json::Value& svlstconf, 
		const Json::Value& sschconf);

	virtual int onListChange(int type, vector<Serv> &servlist);
	virtual int onDisableListChange(int type, vector<int> &servlist){
		return 0;
	}

	int startListen(int port, LogicConFactory*, 
		ConnectionDispatcher* d = NULL);
	int stopListen(int port);

	int run();
	int stop();
	
private:
	int connectServer(const Serv& s, int id);
	int connectIPArray(SvCon* c, const Json::Value& a, int port);
	int allThreadConnectServer(const Serv& s);

	int initServerListCache(const Json::Value& v);

	int initDispatcher(const Json::Value& v);

	int m_thread_cnt;	
	int m_keepalive_span;
	int m_reconnect_span;
	int m_service_type;
	int m_id;
	SvConFactory* m_confac;
	Server m_cliset;
	vector<Serv> m_servlist;
	SvLstCache* m_cache;
	string m_logname;
};



};

#endif
