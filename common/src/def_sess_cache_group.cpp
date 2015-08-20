#include "def_sess_cache_group.h"
#include "base/ef_base64.h"
#include "base/ef_tsd_ptr.h"

namespace gim{
	DefSessCacheGroup::~DefSessCacheGroup(){
	}
	int DefSessCacheGroup::getSession(const std::string &key, std::vector<Sess> &m){
		std::vector<Sess> tm;
		for (unsigned int n=0;n<m_caches.size();++n){
			SessCache* p = m_caches[n];
			if(p)
				p->getSession(key, tm);
		}

		std::set<std::string> sessids;
		for (unsigned int n=0;n<tm.size();++n){
			const Sess& ss = tm[n];
			if(sessids.find(ss.id()) != sessids.end())
				continue;
			sessids.insert(ss.id());
			m.push_back(ss);
		}

		return 0;
	}
	int DefSessCacheGroup::setSession(const Sess &s){
		for (unsigned int n=0;n<m_caches.size();++n){
			SessCache* p = m_caches[n];
			if(p)
				p->setSession(s);
		}
		return 0;
	}
	int DefSessCacheGroup::delSession(const Sess &s){
		for (unsigned int n=0;n<m_caches.size();++n){
			SessCache* p = m_caches[n];
			if(p)
				p->delSession(s);
		}
		return 0;
	}
	void DefSessCacheGroup::addSessCache(SessCache* s){
		m_caches.push_back(s);
	}

	
	DefSsChGrpFactory::~DefSsChGrpFactory(){
		for (unsigned int n=0; n<m_fact.size(); n++){
			delete m_fact[n];
		}
	}

	int DefSsChGrpFactory::init(ZKClient* c, std::vector<std::string>& configpaths){
		int ret = 0;
		for (unsigned int n=0; n<configpaths.size(); n++){
			DefSsChFactory* f = new DefSsChFactory();
			if(0 == f->init(c, configpaths[n])){
				m_fact.push_back(f);
			}
		}
		return ret;
	}
	SessCache* DefSsChGrpFactory::getSessCache(){
		if (!m_cache.get()){
			DefSessCacheGroup* s = new DefSessCacheGroup();
			for (unsigned int n=0;n<m_fact.size();n++){
				DefSsChFactory* f = m_fact[n];
				s->addSessCache(f->getSessCache());
			}
			m_cache.set(s);
		}
		return m_cache.get();
	}
}
