#ifndef _PUSH_COMMON_H_
#define _PUSH_COMMON_H_
#include "base/ef_log.h"
#include <json/json.h>
#include "push_def.h"
#include "base/ef_btype.h"
#include <sstream>
#include "message.pb.h"
#include <stdarg.h>

namespace gim{
using namespace ef;

typedef struct  _JsonArgs
{
	enum{
		ARGEND = 0x0147FFFF,
		ARGMAX = 10
	};
	_JsonArgs(const std::string& k, ...);

	std::string key;
	std::vector<int> types;
}JsonArg;


bool pushRequestJsonCheck(const Json::Value& v);

void messageJsonToProtobuf(const Json::Value& v, Message& msg);
void  messageProtobufToJson(const Message& gm, Json::Value& vm);

int64 strtoi64(const std::string& src);

#define MSG_KEY(cid) (std::string("PM_") + cid)
#define LAST_MSG_ID_KEY(cid) (std::string("PM_") + cid + "_last_msgid")

#define PSLogTrace(a) ef::logTrace(a) << "<addr:" << getIp() << ":" \
	<< getPort() << "> <timestamp:" << gettime_ms() << "> "

#define PSLogError(a) ef::logError(a) << "<addr:" << getIp() << ":" \
	<< getPort() << "> <timestamp:" << gettime_ms() << "> " 


}
#endif //_PUSH_COMMON_H_