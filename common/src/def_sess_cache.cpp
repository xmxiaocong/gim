#include "def_sess_cache.h"
#include "base/ef_base64.h"
#include "zk_client.h"
#include "zk_watcher.h"

namespace gim{
DefSsChFactory::~DefSsChFactory(){
	if(m_w)
		delete m_w;
}
SessCache* DefSsChFactory::getSessCache(){
        DefSessCache*pDBC = m_cache.get();

        if (!pDBC) {
			Json::FastWriter w;
			Json::Value v;
			m_conf.getData(v);
			std::string s = w.write(v);
                pDBC = new DefSessCache(&m_conf);
				m_cache.set(pDBC);
        }

        if(!pDBC)
                return NULL;

        return  pDBC;
}

int DefSsChFactory::init(ZKClient* c, const std::string& wpath){
	m_w = new ZKDataWatcher(wpath);
	m_w->addToClient(c);
	m_w->addWatch(ZKWatcher::CHANGE_EVENT);
	m_w->setCb(this, watchCallBack);
	std::string data;
	if(0 == c->getNodeData(wpath, data)){
		if(0 == setConfig(data))
			return 0;
	}
	return -1;
}

int DefSsChFactory::setConfig(const std::string& data){
	if(data.empty())
		return -1;

	try{
		Json::Value v;
		Json::Reader r;
		if(r.parse(data, v)){
			m_conf.setData(v);
			return 0;
		}
	}
	catch(const std::exception& e){
		return -1;
	}
	return -1;
}

int DefSsChFactory::watchCallBack(void* ctx, int version, const std::string& data){
	DefSsChFactory* __this = (DefSsChFactory*)ctx;
	return __this->setConfig(data);
}

DefSessCache::~DefSessCache(){
	if(m_dbg)
		delete m_dbg;
}

int DefSessCache::init(const Json::Value& config){
	const Json::Value& dbconf = config["DBGroup"];

	if(!dbconf.isObject()){
		return -2001;
	}
	if(m_dbg)
		delete m_dbg;

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
	
	if(m_loader.loadData()){
		init(m_loader.getData());
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
	
	DBHandle h = getHandle(s.id());
	
	if(!h){
		return -1002;
	}

	string str;	
	const_cast<Sess&>(s).set_lasttime(time(NULL));
	s.SerializeToString(&str);	

	int ret = h->hashSet(sessKey(s.id()), s.sessid(), base64Encode(str));
	h->keyExpire(sessKey(s.id()), m_expire);

	return ret;
}

int DefSessCache::delSession(const Sess &s){

	DBHandle h = getHandle(s.id());
	
	if(!h){
		return -1002;
	}

	int ret = h->hashDel(sessKey(s.id()), s.sessid());

	return ret;
}

}
