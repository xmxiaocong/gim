#ifndef __SVLIST_CACHE_H__
#define __SVLIST_CACHE_H__

#include "json/json.h"
#include <vector>

namespace gim{

using namespace std;

//define svlist config field name
#define GIM_LOCAL_IPS "LocalIPs"
#define GIM_PUBLIC_IPS "PublicIPs"
#define GIM_CLIENT_LISTEN_PORT "ClientListenPort"
#define GIM_SERVER_LISTEN_PORT "ServerListenPort"

struct Serv{
	int type;
	int id;
	Json::Value v;
	Serv():type(0), id(0){
	}
};

class SvLstListener{
public:
	virtual ~SvLstListener(){};
	virtual int onListChange(int type, vector<Serv> &servlist) = 0;
	virtual int onDisableListChange(int type, vector<int> &servlist) = 0;
};

class SvLstCache{
public:
	SvLstCache():m_ln(NULL){};
	virtual ~SvLstCache(){};

	void setServerListListener(SvLstListener* ln){
		m_ln = ln;
	}
	
	SvLstListener* getServerListListener(){
		return m_ln;
	}

	virtual int addServer(int type, const Serv &serv) = 0;
	virtual int updateServer(int type, const Serv &serv) = 0;
	virtual int deleteServer(int type, int id) = 0;
	virtual int enableServer(int type, int id) = 0;
	virtual int disableServer(int type, int id) = 0;
	//when watch, may call m_ln->onListChange
	//be careful
	virtual int watchServerList(int type) = 0;
	virtual int unwatchServerList(int type) = 0;

private:
	SvLstListener* m_ln;
};

class SvLstChFactory{
public:
	virtual ~SvLstChFactory(){};
	virtual SvLstCache* getSvLstCache(const Json::Value& config);
};


};

#endif/*CFG_CACHE_H*/
