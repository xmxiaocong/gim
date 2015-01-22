#ifndef __DEF_CACHE_GROUP_H__
#define __DEF_CACHE_GROUP_H__

#include "cache_group.h"

namespace gim{


class RedisCG;

class DefCacheGroup: public CacheGroup{
public:
	DefCacheGroup():m_cg(NULL){};
	
	int init(const Json::Value& conf);

	virtual ~DefCacheGroup();

	/* key related */
	virtual int keyDel(const string &key);
	virtual int keyExists(const string &key, int &exists);
	virtual int keyExpire(const string &key, int seconds);

	virtual int strGet(const string &key, string &value);
	virtual int strGetSet(const string &key, const string &value, 
			string &oldValue);
	virtual int strSet(const string &key, const string &value);

	virtual int intSet(const string &key, int64 v);
	virtual int intGet(const string &key, int64 &v);
	virtual int intIncr(const string &key, int64 &afterIncr);
	virtual int intIncrBy(const string &key, int64 increment, 
			int64 &afterIncr);

	virtual int hashDel(const string &key, const string &field);
  	virtual int hashMDel(const string &key, const vector<string> &fields);
	virtual int hashExists(const string &key, const string &field, 
			int &exists);
	virtual int hashGet(const string &key, const string &field, 
			string &value);
	virtual int hashGetAll(const string &key, map<string, string> &mfv);
	virtual int hashMSet(const string &key, const map<string, string> &mfv);
	virtual int hashSet(const string &key, const string &field, 
			const string &value);


private:
	
	RedisCG* m_cg;

};

}

#endif/*__DEF_CACHE_GROUP_H__*/
