#ifndef __DEF_USER_DB_H__
#define __DEF_USER_DB_H__

#include "user_db.h"
#include "redis_cg.h"

namespace gim{

class DefUserDBFactory: public UserDBFactory{
public:
	DefUserDBFactory(const Json::Value& conf):
		m_conf(conf){
	}

	virtual ~DefUserDBFactory(){}

	virtual UserDB* newUserDB();

private:
	Json::Value m_conf;
};

class DefUserDB:public UserDB{
public:
	DefUserDB():m_dbg(NULL){};
	int init(const Json::Value& conf);
	virtual ~DefUserDB();
	virtual int getUserInfo(const string &cid, map<string, string>& pro);
	virtual int setUserInfo(const string &cid, const map<string, string>& pro);

private:
	
	DBHandle getHandle(const string &key);

	RedisCG* m_dbg;
};

}

#endif/*__DEF_USER_DB_H__*/
