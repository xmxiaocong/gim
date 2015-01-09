#ifndef __DB_CONN_H__
#define __DB_CONN_H__

#include "msg_interface.h"
#include "base/ef_no_copy.h"

namespace gim{
	class DBConn: ef::NoCopy{
	public:
		DBConn();
		~DBConn();
		static MsgInterface* getMsgDB();
		static DBConn* getDBConn(); 
	private:
		int init();
		MsgInterface* m_msgdb;
	};
};

#endif/*CACHE_CONN_H*/
