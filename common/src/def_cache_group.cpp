#include "def_cache_group.h"
#include "redis_cg.h"
#include "base/ef_utility.h"
#include <stdlib.h>

namespace gim{

#define getDBHandle	if(!m_cg){ return -1;} \
			DBHandle h = m_cg->getHndl(key);\
			if(!h){return -2;}

int DefCacheGroup::init(const Json::Value& conf){

	m_cg = new RedisCG(conf);

	if(!m_cg)
		return -1;

	return 0;
}


DefCacheGroup::~DefCacheGroup(){

	if(m_cg){
		delete m_cg;
		m_cg = NULL;
	}
}

int DefCacheGroup::keyDel(const string &key){
	getDBHandle

	return h->keyDel(key);	

}

int DefCacheGroup::keyExists(const string &key, int &exists){
	getDBHandle

	return h->keyExists(key, exists);
}

int DefCacheGroup::keyExpire(const string &key, int seconds){
	getDBHandle

	return h->keyExpire(key, seconds);
}

int DefCacheGroup::strGet(const string &key, string &value){
	getDBHandle

	return h->strGet(key, value);
}

int DefCacheGroup::strGetSet(const string &key, const string &value,
		string &oldValue){
	getDBHandle

	return h->strGetSet(key, value, oldValue);
}

int DefCacheGroup::strSet(const string &key, const string &value){
	getDBHandle

	return h->strSet(key, value);
}

int DefCacheGroup::intSet(const string &key, int64 v){
	getDBHandle

	return h->strSet(key, ef::itostr(v));
}

int DefCacheGroup::intGet(const string &key, int64 &v){
	getDBHandle

	int ret = 0;
	string strv = "0";
	ret = h->strGet(key, strv);
	v = atoll(strv.data());	

	return ret;
}

int DefCacheGroup::intIncr(const string &key, int64 &afterIncr){
	getDBHandle

	return h->strIncr(key, afterIncr);
}

int DefCacheGroup::intIncrBy(const string &key, int64 increment,
		int64 &afterIncr){
	getDBHandle

	return h->strIncrBy(key, increment, afterIncr);
}

int DefCacheGroup::hashDel(const string &key, const string &field){
	getDBHandle

	return h->hashDel(key, field);
}

int DefCacheGroup::hashMDel(const string &key, const vector<string> &fields){
	getDBHandle

	return h->hashMDel(key, fields);
}

int DefCacheGroup::hashExists(const string &key, const string &field,
		int &exists){
	getDBHandle

	return h->hashExists(key, field, exists);
}

int DefCacheGroup::hashGet(const string &key, const string &field,
		string &value){
	getDBHandle

	return h->hashGet(key, field, value);
}

int DefCacheGroup::hashGetAll(const string &key, map<string, string> &mfv){
	getDBHandle

	return h->hashGetAll(key, mfv);
}

int DefCacheGroup::hashMSet(const string &key, const map<string, string> &mfv){
	getDBHandle

	return h->hashMSet(key, mfv);
}


int DefCacheGroup::hashSet(const string &key, const string &field,
		const string &value){
	getDBHandle

	return h->hashSet(key, field, value);

}

}
