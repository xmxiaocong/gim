#ifndef __DEF_SESS_CACHE_H__
#define __DEF_SESS_CACHE_H__

#include "sess_cache.h"
#include "redis_cg.h"
#include "base/ef_loader.h"
#include "base/ef_tsd_ptr.h"

namespace gim{


class DefSessCache: public SessCache{
public:
	enum{
		SESSION_DEFAULT_EXPIRE_SECOND = 500,
	};

	DefSessCache(ef::DataSrc<Json::Value>* config):m_dbg(NULL), 
		m_expire(SESSION_DEFAULT_EXPIRE_SECOND),
		m_loader(config){
		m_loader.loadData();
		init(m_loader.getData());
	};
	virtual ~DefSessCache();
	virtual int getSession(const string &key, vector<Sess> &m);
	virtual int setSession(const Sess &s);
	virtual int delSession(const Sess &s);
private:
	int init(const Json::Value& config);
	
	DBHandle getHandle(const string &key);

	RedisCG* m_dbg;
	int m_expire;
	ef::Loader<Json::Value> m_loader;
};

class ZKClient;
class ZKDataWatcher;

class DefSsChFactory: public SsChFactory{
public:
	DefSsChFactory()
		:m_w(NULL){
	}
	~DefSsChFactory();
	int init(ZKClient* c, const std::string& wpath);
	virtual SessCache* getSessCache();
	static int watchCallBack(void* ctx, int version, const std::string& data);
private:
	int setConfig(const std::string& data);
private:
	ZKDataWatcher* m_w;
	ef::DataSrc<Json::Value> m_conf;
	TSDPtr<DefSessCache> m_cache;
};

}//end of namespace

#endif
