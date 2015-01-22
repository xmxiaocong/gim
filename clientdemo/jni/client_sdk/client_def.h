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
		NOTIFY_TYPE_LOGIN_STATUS_CHANGE = 0,
		NOTIFY_TYPE_PEER_MSG = 200,
		NOTIFY_TYPE_PEER_SEND_RESP = 201,
		NOTIFY_TYPE_PEER_OFFLINE_MSG = 202
	}MsgType;


	typedef struct _GPeerMessage
	{
		int64 id;
		int64 time;
		std::string from;
		std::string to;
		std::string data;
	}GPeerMessage;

};

#endif
