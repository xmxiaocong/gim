#ifndef __REDIS_CG_H__
#define __REDIS_CG_H__

#include "cache_group.h"
#include "redis_client.h"

namespace gim {

struct DBServ {
        string ipaddr;
        int port;
        string passwd;
};

class RedisCG : public CacheGroup {
public:
	~RedisCG();
	int init(const vector<string> &servList);
	int clear();
	DBHandle getHndl(const string &key);
private:
	vector <DBHandle> m_dbs;
	vector <DBServ> m_servList;
};

class RdsCGFactory : public CacheGroupFactory {
public:
	CacheGroup *createNewCG(const Json::Value &config)
	{
		CacheGroup *c = new RedisCG();
		return c;
	}
};

};
#endif
