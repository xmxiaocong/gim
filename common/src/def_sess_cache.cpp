#include "def_sess_cache.h"
#include "base/ef_base64.h"


namespace gim{

DefSessCache::~DefSessCache(){
	if(m_dbg)
		delete m_dbg;
}

int DefSessCache::init(const Json::Value& config){
	m_dbg = new RedisCG(config);
	return 0;
}

inline string sessKey(const string& key){
	return "SESS_" + key;
}

DBHandle DefSessCache::getHandle(const string &key){
	if(!m_dbg){
		return NULL;
	}

	DBHandle h = m_dbg->getHndl(sessKey(key));

	return h;
}


int DefSessCache::getSession(const string &key, vector<Sess> &m){

	DBHandle h = getHandle(key);
	
	if(!h){
		return -1002;
	}

	map<string, string> sm;

	int ret = h->hashGetAll(sessKey(key), sm);
	
	map<string, string>::iterator it = sm.begin();
	
	for(; it != sm.end(); ++it){
		Sess s;
		string str = base64Decode(it->second);
		s.ParseFromString(str);
		m.push_back(s);
	}

	if(ret == CACHE_NOT_EXIST){
		ret = 0;
	}
		
	return ret;

}

int DefSessCache::setSession(const Sess &s){
	
	DBHandle h = getHandle(s.cid());
	
	if(!h){
		return -1002;
	}

	string str;	

	s.SerializeToString(&str);	

	int ret = h-> hashSet(sessKey(s.cid()), s.sessid(), base64Encode(str));

	return ret;
}

int DefSessCache::delSession(const Sess &s){

	DBHandle h = getHandle(s.cid());
	
	if(!h){
		return -1002;
	}

	int ret = h-> hashDel(sessKey(s.cid()), s.sessid());

	return ret;
}

}
