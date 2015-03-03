#ifndef __DEF_SVLIST_CACHE_H__
#define __DEF_SVLIST_CACHE_H__


#include "svlist_cache.h"
#include <map>

namespace gim{

class DefSvLstChFactory: public SvLstChFactory{
public:
	DefSvLstChFactory(const Json::Value& conf):
		m_conf(conf){
	}

	virtual ~DefSvLstChFactory(){}

	SvLstCache* newSvLstCache();

private:
	Json::Value m_conf;
};

class DefSvLstCache:public SvLstCache{
public:
	virtual ~DefSvLstCache(){};
	int init(const Json::Value& config);


	virtual int addServer(int type, const Serv &serv);
	virtual int updateServer(int type, const Serv &serv);
	virtual int deleteServer(int type, int id);
	virtual int enableServer(int type, int id);
	virtual int disableServer(int type, int id);
	virtual int watchServerList(int type);
	virtual int unwatchServerList(int type);
private:
	typedef vector<Serv> svlst_t;

	int loadOneType(const Json::Value& config);
	int loadOneServer(int type, const Json::Value& config);

	map<int, svlst_t> m_svlsts;	
	map<int, SvLstListener*> m_listeners;
};

}


#endif
