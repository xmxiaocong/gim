#ifndef __SVLIST_CACHE_H__
#define __SVLIST_CACHE_H__

#include "json/json.h"
#include <vector>

namespace gim{

using namespace std;

struct Serv{
	int type;
	int id;
	Json::Value v;
	Serv():type(0), id(0){
	}
};

class SvlstCache{
public:
	virtual int getEnableList(int type, vector<Serv> &servlist) = 0;
	virtual int getDisableSvIDList(int type, vector<int> &servlist) = 0;
	virtual int getAllList(int type, vector<Serv> &servlist) = 0;
	virtual int addServ(int type, const Serv &serv) = 0;
	virtual int updateServ(int type, const Serv &serv) = 0;
	virtual int deleteServ(int type, int id) = 0;
	virtual int enableServ(int type, int id) = 0;
	virtual int disableServ(int type, int id) = 0;
};

class DefSvlstCache{
public:
	virtual int getEnableList(int type, vector<Serv> &servlist) = 0;
	virtual int getDisableSvIDList(int type, vector<int> &servlist) = 0;
	virtual int getAllList(int type, vector<Serv> &servlist) = 0;
	virtual int addServ(int type, const Serv &serv) = 0;
	virtual int updateServ(int type, const Serv &serv) = 0;
	virtual int deleteServ(int type, int id) = 0;
	virtual int enableServ(int type, int id) = 0;
	virtual int disableServ(int type, int id) = 0;
private:
};

};

#endif/*CFG_CACHE_H*/
