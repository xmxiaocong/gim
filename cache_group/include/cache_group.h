#ifndef __CACHE_GROUP_H__
#define __CACHE_GROUP_H__

#include <string>
#include <vector>
#include <map>
#include "json/json.h"
#include "base/ef_btype.h"
#include "gtype.h"

namespace gim {

using namespace ef;
using namespace std;

class CacheGroup {
public:
	CacheGroup():m_cmdLog(NULL){};
	~CacheGroup(){};
	void setCmdLog(LogCb cb){m_cmdLog = cb;};
protected:
	LogCb m_cmdLog;
};

class CacheGroupFactory {
public:
	virtual CacheGroup *createNewCG(const Json::Value &config)
	{
		return NULL;
	}
};
uint64 getIdx(int size, const string &key);

}

#endif // __CACHE_GROUP_H__
