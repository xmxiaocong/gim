#ifndef __SESS_CACHE_H__
#define __SESS_CACHE_H__

#include <string>
#include <vector>

namespace gim{

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

}

#endif
