#ifndef MSG_HEAD_H
#define MSG_HEAD_H
#include "common/ef_btype.h"

using namespace ef;

namespace gim
{
	enum
	{
		KEEPALIVE_REQ = 0,
		KEEPALIVE_RESP = KEEPALIVE_REQ + 1,
		LOGIN_REQ = 100,
		LOGIN_RESP = LOGIN_REQ + 1,
		SERVICE_CMD_REQ = 200,
		SERVICE_CMD_RESP = SERVICE_CMD_REQ + 1,
		SERVICE_REG_REQ = 300,
		SERVICE_REG_RESP = SERVICE_REG_REQ + 1,
		KICK_CLIENT = 400,
		REDIRECT_RESP = 1001,
		SET_TIME_RESP = 1101,
		JSON_PUSH_REQ = 9999,
		JSON_PUSH_RESP = JSON_PUSH_REQ + 1,
		SERVER_CMD_SERVICE_REQ = 6666,
		SERVER_CMD_TOPIC_REQ = SERVER_CMD_SERVICE_REQ,
		SERVER_SERVICE_RESP = SERVER_CMD_SERVICE_REQ + 1
	};

	struct head
	{
		int32 magic;
		int32 len;//include head 
		//0: keepalive, 1: keepalive resp
		//100: login, 101: login resp
		//200: service, 201: service resp
		//300: regist service 301: regist service resp
		//400: kick client
		//1001: redericet
		//9999:json push request 10000:json push response
		int32 cmd;
	};
};

#endif
