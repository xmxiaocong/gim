#ifndef _PUSH_MSG_DB_H_
#define _PUSH_MSG_DB_H_
#include <json/json.h>
#include "base/ef_btype.h"
#include <vector>
#include "redis_msg_q.h"

namespace gim
{
	class PushMsgDBM
	{
	public:
		static void initConfig(const Json::Value& conf);
		static RedisMQ* MQ();
	private:
		static Json::Value g_push_db_cfg;
	};
}

#endif