#include "db_conn.h"
#include "settings.h"
#include "base/ef_tsd_ptr.h"

namespace gim{

TSDPtr<DBConn> g_DbConn;

DBConn::DBConn():m_msgdb(NULL){
}

int DBConn::init(){
	Settings *pSettings = Singleton<Settings>::instance();


	MsgInterfaceFactory mdf;
	m_msgdb = mdf.newMsgInterface(pSettings->MsgDBConfig); 

	return 0;
}

DBConn::~DBConn(){
}

DBConn* DBConn::getDBConn()
{
	DBConn *pDBC = g_DbConn.get();
	if (!pDBC) {
		pDBC = new DBConn();
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

MsgInterface* DBConn::getMsgDB()
{
	DBConn *pDBC = getDBConn();

	if(!pDBC)
		return NULL;

	return pDBC->m_msgdb;
}
	
};
