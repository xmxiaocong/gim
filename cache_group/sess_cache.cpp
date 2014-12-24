#include "sess_cache.h"

namespace gim{

int DefSessCache::getSession(const string &key, vector<Sess> &m){
	int ret = 0;
	return ret;
}

int DefSessCache::setSession(const Sess &s){
	int ret = 0;
	return ret;
}

int DefSessCache::delSession(const Sess &s){
	int ret = 0;
	return ret;
}

SessCache* SsChFactory::getSessCache(const Json::Value& config){
	return NULL;
}

}
