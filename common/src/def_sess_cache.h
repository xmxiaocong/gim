#ifndef __DEF_SESS_CACHE_H__
#define __DEF_SESS_CACHE_H__

#include "sess_cache.h"
#include "redis_cg.h"

namespace gim{

class DefSsChFactory: public SsChFactory{
public:
	DefSsChFactory(const Json::Value& conf)
		:m_conf(conf){
	}

	~DefSsChFactory(){}

	virtual SessCache* newSessCache();
	
private:
	Json::Value m_conf;
};

class DefSessCache: public SessCache{
public:
	enum{
		SESSION_DEFAULT_EXPIRE_SECOND = 500,
	};

	DefSessCache():m_dbg(NULL), 
		m_expire(SESSION_DEFAULT_EXPIRE_SECOND){};
	virtual ~DefSessCache();
	int init(const Json::Value& config);
	virtual int getSession(const string &key, vector<Sess> &m);
	virtual int setSession(const Sess &s);
	virtual int delSession(const Sess &s);
private:
	
	DBHandle getHandle(const string &key);

	RedisCG* m_dbg;
	int m_expire;
};


}

#endif
