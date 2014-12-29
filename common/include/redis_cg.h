#ifndef __REDIS_CG_H__
#define __REDIS_CG_H__

#include "redis_client.h"
#include "json/json.h"

namespace gim {

struct DBServ {
        string ipaddr;
        int port;
        string passwd;
};

struct RedisCGCfg {
	vector <DBServ> servList;
};

class RedisCG {
public:
	RedisCG(const Json::Value &config, LogCb cb = NULL);
	~RedisCG();
	int clear();
	DBHandle getHndl(const string &key);
	void setCmdLog(LogCb cb);
private:
	int init(const Json::Value &config);

	vector <DBHandle> m_dbs;
	RedisCGCfg m_cfg;
	LogCb m_cmdLog;
};

};
#endif
