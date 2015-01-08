#ifndef __DEF_SVLIST_CACHE_H__
#define __DEF_SVLIST_CACHE_H__


#include "svlist_cache.h"
#include <map>

namespace gim{

class DefSvLstCache:public SvLstCache{
public:
	virtual ~DefSvLstCache(){};
	int init(const Json::Value& config);

	virtual int watchServerList(int type){
		return 0;
	}

	virtual int unwatchServerList(int type){
		return 0;
	}

	virtual int addServer(int type, const Serv &serv);
	virtual int updateServer(int type, const Serv &serv);
	virtual int deleteServer(int type, int id);
	virtual int enableServer(int type, int id);
	virtual int disableServer(int type, int id);
private:
	typedef vector<Serv> svlst_t;

	int loadOneType(const Json::Value& config);
	int loadOneServer(int type, const Json::Value& config);

	map<int, svlst_t> m_svlsts;	
	map<int, SvLstListener*> m_listeners;
};

}


#endif
