#ifndef __CACHE_CONN_H__
#define __CACHE_CONN_H__


#include "sess_cache.h"
#include "user_db.h"
#include "base/ef_no_copy.h"

namespace gim{
	class CacheConn: ef::NoCopy{
	public:
		CacheConn();
		~CacheConn();
		static SessCache* getSessCache();	
		static UserDB* getUserDB();
		static CacheConn* getCacheConn(); 
	private:
		int init();
		SessCache* m_cache;
		UserDB* m_userdb;
	};
};

#endif/*CACHE_CONN_H*/
