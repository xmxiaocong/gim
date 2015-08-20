#ifndef _PUSH_DEF_H_
#define _PUSH_DEF_H_
namespace gim{
	enum{
		PS_RESULT_OK = 0,
		PS_RESULT_ERROR = -1,
		PS_RESULT_JSON_ERROR = -2,
		PS_RESULT_ARGS_ERROR = -3, 
		PS_RESULT_UNDEFINED_CMD = -4,
		PS_RESULT_INCR_MSGID_ERROR = -5,
		PS_RESULT_REDIS_ERROR = -6
	};

	enum{
		PS_CMD_CALL_PUSH = 11, //call push to send msg to client
		PS_CMD_CALL_PUSH_RESP = 12,

		PS_CMD_PUSH_REQUEST = 1, //push msg
		PS_CMD_PUSH_RESPONSE = 2, //push msg response
		PS_CMD_GET_PUSH_MSG = 3,
		PS_CMD_GET_PUSH_MSG_RESPONSE=4,
		PS_CMD_PEER_REQUEST = 5,
		PS_CMD_PEER_RESPONSE = 6
	};
}
#endif //_PUSH_DEF_H_