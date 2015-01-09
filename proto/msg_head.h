#ifndef __MSG_HEAD_H__
#define __MSG_HEAD_H__

#include "base/ef_btype.h"

namespace gim{

using namespace ef;

enum{
	KEEPALIVE_REQ = 0,
	KEEPALIVE_RESP = KEEPALIVE_REQ + 1,
	LOGIN_REQ = 100,
	LOGIN_RESP = LOGIN_REQ + 1,
	SERVICE_REQ = 200,
	SERVICE_RESP = SERVICE_REQ + 1,
	SERVICE_REG_REQ = 300,
	SERVICE_REG_RESP = SERVICE_REG_REQ + 1,
	KICK_CLIENT = 400,
	REDIRECT_RESP = 1001,	
	SET_TIME_RESP = 1101,
	MAGIC_NUMBER = 0x20141228,
};

struct head{
	int32 magic;
	int32 len;//include head 
	//0: keepalive, 1: keepalive resp
	//100: login, 101: login resp
	//200: service, 201: service resp
	//300: regist service 301: regist service resp
	//400: kick client
	//1001: redericet
	int32 cmd;			
};


};

#endif
