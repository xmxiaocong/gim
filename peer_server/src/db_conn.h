#ifndef __DB_CONN_H__
#define __DB_CONN_H__

#include "msg_db.h"
#include "base/ef_no_copy.h"

namespace gim{
	class DBConn: ef::NoCopy{
	public:
		DBConn();
		~DBConn();
		static MsgDB* getMsgDB();
		static DBConn* getDBConn(); 
	private:
		int init();
		MsgDB* m_msgdb;
	};
};

#endif/*CACHE_CONN_H*/
