#include "def_user_db.h"
#include "base/ef_base64.h"


namespace gim{

UserDB* DefUserDBFactory::newUserDB(){
	DefUserDB* c = new DefUserDB();

	if(c->init(m_conf) >= 0){
		return c;
	}

	delete c;
	return NULL;
}

DefUserDB::~DefUserDB(){
	if(m_dbg)
		delete m_dbg;
}

int DefUserDB::init(const Json::Value& config){
	const Json::Value& dbconf = config["DBGroup"];

	if(!dbconf.isObject()){
		return -2001;
	}

	m_dbg = new RedisCG(dbconf);


	return 0;
}

inline string userKey(const string& cid){
	return "USER_" + cid;
}

DBHandle DefUserDB::getHandle(const string &cid){
	if(!m_dbg){
		return NULL;
	}

	DBHandle h = m_dbg->getHndl(userKey(cid));

	return h;
}


int DefUserDB::getUserInfo(const string &cid, map<string, string> &pro){

	DBHandle h = getHandle(cid);
	
	if(!h){
		return -1002;
	}


	int ret = h->hashGetAll(userKey(cid), pro);

	if(ret == CACHE_NOT_EXIST){
		ret = 0;
	}
		
	return ret;

}

int DefUserDB::setUserInfo(const string &cid, const map<string, string> &pro){
	
	DBHandle h = getHandle(cid);
	
	if(!h){
		return -1002;
	}

	string str;	

	int ret = h->hashMSet(userKey(cid), pro);

	return ret;
}

}
