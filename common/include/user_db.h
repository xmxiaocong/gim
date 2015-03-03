#ifndef __USER_DB_H__
#define __USER_DB_H__

#include <map>
#include <string>
#include "json/json.h"
#include "base/ef_btype.h"

namespace gim{

using namespace std;
using namespace ef;


class UserDB{
public:
	virtual ~UserDB(){}
	virtual int getUserInfo(const string &cid, map<string, string>& pro) = 0;
	virtual int setUserInfo(const string &cid, const map<string, string>& pro) = 0;
};

class UserDBFactory{
public:
	virtual ~UserDBFactory(){};
	virtual UserDB* newUserDB() = 0;
	
	static int init(const Json::Value& conf);
	static void free();
	static UserDBFactory* get();
	static UserDBFactory* create(const Json::Value& conf);
private:
	static UserDBFactory* g_fct;
};

UserDBFactory* newUserDBFactory(const Json::Value& config);

}

#endif
