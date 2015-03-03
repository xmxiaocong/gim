#include "cache_conn.h"
#include "settings.h"
#include "base/ef_tsd_ptr.h"

namespace gim{

TSDPtr<CacheConn> g_DbConn;

CacheConn::CacheConn(){
}

int CacheConn::init(){
	Settings *pSettings = Singleton<Settings>::instance();

	SsChFactory* ssf = SsChFactory::get();
	m_cache = ssf->newSessCache();

	UserDBFactory* udbf = UserDBFactory::get();
	m_userdb = udbf->newUserDB(); 

	return 0;
}

CacheConn::~CacheConn(){
}

CacheConn*
CacheConn::getCacheConn()
{
	CacheConn *pDBC = g_DbConn.get();
	if (!pDBC) {
		pDBC = new CacheConn();
		if(pDBC->init() >= 0){
			g_DbConn.set(pDBC);
		}else{
			delete pDBC;
			pDBC = NULL;
		}
	}

	if(!pDBC)
		return NULL;

	return	pDBC;
}


SessCache*
CacheConn::getSessCache()
{
	CacheConn *pDBC = getCacheConn();

	if(!pDBC)
		return NULL;

	return	pDBC->m_cache;
}

UserDB*
CacheConn::getUserDB()
{
	CacheConn *pDBC = getCacheConn();

	if(!pDBC)
		return NULL;

	return pDBC->m_userdb;
}
	
};
