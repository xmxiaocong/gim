#ifndef __SESS_CACHE_H__
#define __SESS_CACHE_H__

#include <string>
#include <vector>
#include "session.pb.h"
#include "json/json.h"
#include "base/ef_btype.h"
#include "base/ef_thread.h"
#include "base/ef_loader.h"

namespace gim{

class ZKClient;

class SessCache{
public:
	virtual ~SessCache(){}
	virtual int getSession(const std::string &key, std::vector<Sess> &m) = 0;
	virtual int setSession(const Sess &s) = 0;
	virtual int delSession(const Sess &s) = 0;
};

class SsChFactory{
public:
	virtual ~SsChFactory(){}

	virtual SessCache* getSessCache() = 0;

	static int initSsChFactory(ZKClient* zkc, const Json::Value& conf);
	static int freeSsChFactory();
	static SsChFactory* getSsChFactory();
private:
	static SsChFactory* g_fact;
};



}

#endif
