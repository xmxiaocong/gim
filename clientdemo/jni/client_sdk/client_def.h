#ifndef CLIENT_DEF_H
#define CLIENT_DEF_H

#include "common/ef_btype.h"
#include <string>
#include <vector>

namespace gim
{
	using namespace ef;
	typedef enum _LoginStatus
	{
		STATUS_DO_LOGIN = 1,
		STATUS_LOGIN = 2,
		STATUS_LOGIN_FAIL = 3,
		STATUS_DISCONNECT = 4
	}LoginStatus;

	typedef enum _GResult
	{
		MY_OK = 0,
		MY_ERROR = -1,
		MY_NETWORK_ERROR = -9999,
		MY_PROBUF_FORMAT_ERROR = -9998,
		MY_TOO_LONG_PACKET = -9997,
		MY_JNI_ERROR = -9996,
		MY_JSON_ERROR = -9995,
		MY_UNDEFINED_CMD = -9994,
		MY_NOT_LOGGED = -9993,
		MY_TIMEOUT	= -9992
	}GResult;


	typedef enum _MsgType
	{
		MSG_TYPE_PUSH = 1,
		MSG_TYPE_OFFLINE_PUSH_RESP = 2,

		MSG_TYPE_PEER = 10,
		MSG_TYPE_SEND_PEER_RESP = 11,
		MSG_TYPE_OFFLINE_PEER_RESP = 12,

		MSG_TYPE_GET_VERIFY_CODE_RESP = 30,
		MSG_TYPE_ANTH_VERIFY_CODE_RESP = 31,

		MSG_TYPE_LOGIN_STATUS_CHANGE = 40,

		MSG_TYPE_POST_TOPIC_RESP = 50,
		MSG_TYPE_REPLY_TOPIC_RESP = 51,

		MSG_TYPE_ADD_BLOCK_USERS_RESP = 60,
		MSG_TYPE_REMOVE_BLOCK_USERS_RESP = 61,
		MSG_TYPE_GET_BLOCK_USERS_RESP = 62,
		MSG_TYPE_PUBLIC_SERVICE_RESP = 63,
	}MsgType;

	typedef enum _MsgCmd
	{
		MSG_CMD_LOGIN = 1,
		MSG_CMD_DISCONNECT = 2,
		MSG_CMD_SEND_PEER_MSG = 3,
		MSG_CMD_GET_VERIFY_CODE = 4,
		MSG_CMD_ADD_BLOCK_USERS = 5,
		MSG_CMD_REMOVE_BLOCK_USERS = 6,
		MSG_CMD_GET_BLOCK_USERS = 7,
		MSG_CMD_PUBLIC_SERVICE = 8,
	}MsgCmd;

	typedef struct _GPeerMessage
	{
		int64 id;
		int64 time;
		std::string from;
		std::string to;
		std::string data;
	}GPeerMessage;


#define JKEY_CID "cid"
#define JKEY_MSG_TYPE "type" 				 //msg type
#define JKEY_MSG_SN "sn"      				//request serial number
#define JKEY_MSG_STATUS "status"			//request status when >=0, request succeed
#define JKEY_MSG_TO   "to"					//msg send to 
#define JKEY_MSG_FROM "from"				//msg received from
#define JKEY_MSG_TIME "time"				//msg time
#define JKEY_MSG_ID "id"					//msg id
#define JKEY_MSG_DATA "data"				//msg content

#define JKEY_MSG_VERIFY_CODE "vercode"		//verify code, request it for post topic
#define JKEY_MSG_VERIFY_RETCODE	"verify_retcode"	//verify return code, when =0, request succeed

#define JKEY_MSG_LOGIN_STATUS "login_status"		//login status, defined  in enum LoginStatus
#define JKEY_MSG_ARRAY "msgs"						//offline msg array
#define JKEY_MSG_OFFLINE_LEFTSIZE "offline_leftsize" //offline msg left size

#define JKEY_MSG_BLOCK_USERS "block_users"			//block users
#define JKEY_MSG_BLOCK_VERSION "block_version"		//block users version, when updated, version increase

#define JKEY_MSG_CMD "cmd"

#define JKEY_MSG_TOPIC_TYPE "topic_type"
#define JKEY_MSG_TOPIC_DATA "topic_data"
#define JKEY_MSG_PUBLIC_SERVICE "public_service"
#define JKEY_MSG_PUBLIC_SERVICE_DATA "public_service_data"

#define JKEY_VERIFY_CODE_VALID "valid"   //times code can used,
#define JKEY_VERIFY_CODE_EXPIRE "expire" //expiretime, in seconds
#define JKEY_VERIFY_CODE		"code"	 //verify code

};

#endif
