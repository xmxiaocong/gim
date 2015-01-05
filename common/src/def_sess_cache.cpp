#include "def_sess_cache.h"
#include "base/ef_base64.h"


namespace gim{

DefSessCache::~DefSessCache(){
	if(m_dbg)
		delete m_dbg;
}

int DefSessCache::init(const Json::Value& config){
	const Json::Value& dbconf = config["DBGroup"];

	if(!dbconf.isObject()){
		return -2001;
	}

	m_dbg = new RedisCG(dbconf);

	const Json::Value& exp = config["Expire"];

	if(exp.isInt()){
		m_expire = exp.asInt();
	}

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

	time_t n = time(NULL);
	map<string, string> sm;

	int ret = h->hashGetAll(sessKey(key), sm);
	vector<string> oldsessids;
	map<string, string>::iterator it = sm.begin();	

	for(; it != sm.end(); ++it){
		Sess s;
		string str = base64Decode(it->second);
		s.ParseFromString(str);

		if(s.lasttime() + m_expire < n){
			oldsessids.push_back(s.sessid());
		}else{	
			m.push_back(s);
		}
	}

	if(oldsessids.size()){
		h->hashMDel(sessKey(key), oldsessids);
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
	const_cast<Sess&>(s).set_lasttime(time(NULL));
	s.SerializeToString(&str);	

	int ret = h->hashSet(sessKey(s.cid()), s.sessid(), base64Encode(str));
	h->keyExpire(sessKey(s.cid()), m_expire);

	return ret;
}

int DefSessCache::delSession(const Sess &s){

	DBHandle h = getHandle(s.cid());
	
	if(!h){
		return -1002;
	}

	int ret = h->hashDel(sessKey(s.cid()), s.sessid());

	return ret;
}

}
