#ifndef __SESS_CACHE_H__
#define __SESS_CACHE_H__

#include <string>
#include <vector>
#include "json/json.h"
#include "base/ef_btype.h"

namespace gim{

using namespace std;
using namespace ef;

struct Sess{
	string	cid;
	int64	lasttime;//last act time
	string	sessid;
	int	svid;//connect server id
	Sess()
		:lasttime(0){
	}
};

class SessCache{
public:
	virtual ~SessCache(){}
	virtual int getSession(const string &key, vector<Sess> &m) = 0;
	virtual int setSession(const Sess &s) = 0;
	virtual int delSession(const Sess &s) = 0;
};

class DefSessCache{
public:
	virtual int getSession(const string &key, vector<Sess> &m);
	virtual int setSession(const Sess &s);
	virtual int delSession(const Sess &s);
private:
};

class SsChFactory{
public:
	virtual SessCache* getSessCache(const Json::Value& config);
};

}

#endif
