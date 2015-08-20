#ifndef __DEF_SESS_CACHE_CLUSTER_H__
#define __DEF_SESS_CACHE_CLUSTER_H__

#include "sess_cache.h"
#include "def_sess_cache.h"
#include "redis_cg.h"
#include "base/ef_loader.h"
#include <set>
#include <vector>
#include "base/ef_tsd_ptr.h"

namespace gim{
	class DefSessCacheGroup:public SessCache{
	public:
		DefSessCacheGroup(){};
		virtual ~DefSessCacheGroup();
		virtual int getSession(const std::string &key, std::vector<Sess> &m);
		virtual int setSession(const Sess &s);
		virtual int delSession(const Sess &s);
		void addSessCache(SessCache* s);
	private:
		std::vector<SessCache*> m_caches;
	};
	
	class DefSsChGrpFactory:public SsChFactory{
	public:
		DefSsChGrpFactory(){};
		virtual ~DefSsChGrpFactory();

		int init(ZKClient* c, std::vector<std::string>& configpaths);
		virtual SessCache* getSessCache();
	private:
		std::vector<DefSsChFactory*> m_fact;
		TSDPtr<DefSessCacheGroup> m_cache;
	};
}//end of namespace

#endif
