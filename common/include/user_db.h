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
	virtual UserDB* getUserDB(const Json::Value& config);
};

}

#endif
