#ifndef __REDIS_MSG_INTERFACE_H__
#define __REDIS_MSG_INTERFACE_H__

#include "msg_interface.h"
#include "redis_cg.h"

namespace gim {

using namespace std;
using namespace ef;

#define DEFAULT_MSG_BOX_CAPACITY 100
#define DEFAULT_MSG_EXPIRY_TIME  60 * 60 * 24 * 7 /* a week */

class RedisMI : public MsgInterface {
public:
	RedisMI(const Json::Value &config);

	// Caution! You must bind a valid RedisCG handler before any other operation
	int bindCG(RedisCG *cg);

	int size(const string &mbName);
	
	int sizeFrom(const string &mbName, int64 bMsgId);

	int addMsg(const string &mbName, Message &msg);

	int getMsgs(const string &mbName, int64 bMsgId, int length,
			vector<Message> &msg);

	int getMsgsForward(const string &mbName, 
			int64 bMsgId, int length, vector<Message> &msg);

	int delMsg(const string &mbName, int64 msgId);

	int delMsgsBackward(const string &mbName, int64 bMsgId, int length = -1);

	int delMsgs(const string &mbName, int64 bMsgId, int length = -1);

	int clear(const string &mbName);

	int incrId(const string &mbName, int64 &newId);

	int getMsgId(const string &mbName, int64 &recentReadId);

	int setMsgId(const string &mbName, int64 recentReadId);
private:
	RedisCG *m_cg;

	Json::Value m_cfg;

	int m_expiry;

	int m_capacity;
};

class RedisMIFactory : public MsgInterfaceFactory {
public:
	MsgInterface *newMsgInterface(const Json::Value &config)
	{
		return new RedisMI(config);
	}
};

}
	

#endif //__REDIS_ADAPTOR_H__
