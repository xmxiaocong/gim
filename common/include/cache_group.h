#ifndef __CACHE_GROUP_H__
#define __CACHE_GROUP_H__

#include <map>
#include <string>
#include <vector>
#include "json/json.h"
#include "base/ef_btype.h"

namespace gim{

using namespace std;
using namespace ef;

class CacheGroup{
public:

	virtual ~CacheGroup(){};

	/* key related */
	virtual int keyDel(const string &key) = 0;
	virtual int keyExists(const string &key, int &exists) = 0;
	virtual int keyExpire(const string &key, int seconds) = 0;

	virtual int strGet(const string &key, string &value) = 0;
	virtual int strGetSet(const string &key, const string &value, 
			string &oldValue) = 0;
	virtual int strSet(const string &key, const string &value) = 0;

	virtual int intSet(const string &key, int64 v) = 0;
	virtual int intGet(const string &key, int64 &v) = 0;
	virtual int intIncr(const string &key, int64 &afterIncr) = 0;
	virtual int intIncrBy(const string &key, int64 increment, 
			int64 &afterIncr) = 0;

	virtual int hashDel(const string &key, const string &field) = 0;
  	virtual int hashMDel(const string &key, const vector<string> &fields) = 0;
	virtual int hashExists(const string &key, const string &field, 
			int &exists) = 0;
	virtual int hashGet(const string &key, const string &field, 
			string &value) = 0;
	virtual int hashGetAll(const string &key, map<string, string> &mfv) = 0;
	virtual int hashMSet(const string &key, const map<string, string> &mfv) = 0;
	virtual int hashSet(const string &key, const string &field, 
			const string &value) = 0;


private:
};

class CacheGroupFactory{
public:
        virtual ~CacheGroupFactory(){};
        virtual CacheGroup* newCacheGroup() = 0;
	static int init(const Json::Value& config);
	static void free();
	static CacheGroupFactory* get();
	static CacheGroupFactory* create(const Json::Value& config);
private:
	static CacheGroupFactory* g_fac;
};


}


#endif
