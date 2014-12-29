#ifndef __DEF_SESS_CACHE_H__
#define __DEF_SESS_CACHE_H__

namespace gim{

class DefSessCache: public SessCache{
public:
	int init(const Json::Value& config);
	virtual int getSession(const string &key, vector<Sess> &m);
	virtual int setSession(const Sess &s);
	virtual int delSession(const Sess &s);
private:
};


}

#endif
