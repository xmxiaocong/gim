#ifndef __DEF_SESS_CACHE_H__
#define __DEF_SESS_CACHE_H__

#include "sess_cache.h"
#include "redis_cg.h"

namespace gim{

class DefSessCache: public SessCache{
public:
	DefSessCache():m_dbg(NULL){};
	virtual ~DefSessCache();
	int init(const Json::Value& config);
	virtual int getSession(const string &key, vector<Sess> &m);
	virtual int setSession(const Sess &s);
	virtual int delSession(const Sess &s);
private:
	
	DBHandle getHandle(const string &key);

	RedisCG* m_dbg;
};


}

#endif
