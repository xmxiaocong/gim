#ifndef __REDIS_MSG_DB_H__
#define __REDIS_MSG_DB_H__

#include "msg_db.h"
#include "redis_cg.h"

namespace gim {

using namespace std;
using namespace ef;

#define DEFAULT_MSG_BOX_CAPACITY 100
#define DEFAULT_MSG_EXPIRY_TIME  60 * 60 * 24 * 7 /* a week */

class RedisMI : public MsgDB {
public:
	RedisMI(const Json::Value &config);
	~RedisMI();

	// returns message numbers in the specified msgbox
	int size(const string &mbName);
	
	// returns amount of messages between lbId and ubId
	// lbId: lower bound. 0 means the oldest message
	// ubId: upper bound. -1 means the latest message
	int count(const string &mbName, int64 lbId, int64 ubId);

	int addMsg(const string &mbName, const Message &msg);

	// get messages whose ids are not less than bMsgId(contains bMsgId itself)
	// bMsgId: the begin message id. 0 means the oldest message
	// length: amount of messages to get. -1 means to the latest message
	int getMsgs(vector<Message> &vMsg, const string &mbName, int64 bMsgId, 
		int length, order_t orderBy);

	// get messages whose ids are not larger than bMsgId(contains eMsgId itself)
	// eMsgId: the end message id. -1 means the latest message
	// length: amount of messages to get. -1 means from the first message;
	int getMsgsForward(vector<Message> &vMsg, const string &mbName, int64 eMsgId, 
			int length, order_t orderBy);

	int delMsg(const string &mbName, int64 msgId);

	// delete messages whose ids are not less than bMsgId(contains bMsgId itself)
	// bMsgId: the begin message id. 0 means the oldest message
	// length: amount of message to delete. -1 means to the latest message
	int delMsgsBackward(const string &mbName, int64 bMsgId, int length);

	// delete messages whose ids are not larger than eMsgId(contains eMsgId itself)
	// eMsgId: the end message id. -1 means the latest message
	// length: amount of message to delete. -1 means from the first message
	int delMsgs(const string &mbName, int64 eMsgId, int length);

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

}
	

#endif //__REDIS_ADAPTOR_H__
