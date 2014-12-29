#ifndef __DEF_SVLIST_CACHE_H__
#define __DEF_SVLIST_CACHE_H__


#include "svlist_cache.h"
#include <map>

namespace gim{

class DefSvLstCache:public SvLstCache{
public:
	virtual ~DefSvLstCache(){};
	int init(const Json::Value& config);
	virtual int getEnableList(int type, vector<Serv> &servlist);
	virtual int getDisableSvIDList(int type, vector<int> &servlist);
	virtual int getAllList(int type, vector<Serv> &servlist);
	virtual int addServ(int type, const Serv &serv);
	virtual int updateServ(int type, const Serv &serv);
	virtual int deleteServ(int type, int id);
	virtual int enableServ(int type, int id);
	virtual int disableServ(int type, int id);
private:
	typedef vector<Serv> svlst_t;

	int loadOneType(const Json::Value& config);
	int loadOneServer(int type, const Json::Value& config);

	map<int, svlst_t> m_svlsts;	
};

}


#endif
