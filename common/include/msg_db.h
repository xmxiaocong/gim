#ifndef __MSG_DB_H__
#define __MSG_DB_H__

#include <string>
#include <vector>
#include "json/json.h"
#include "base/ef_btype.h"
#include "message.pb.h"

namespace gim {

using namespace std;
using namespace ef;

typedef enum {
	ORDER_BY_INCR,
	ORDER_BY_DECR
}order_t;

class MsgDB {
public:
	virtual ~MsgDB(){};

	// returns message numbers in the specified msgbox
	virtual int size(const string &mbName) = 0;
	
	// returns amount of messages between lbId and ubId
	// lbId: lower bound. 0 means the oldest message
	// ubId: upper bound. -1 means the latest message
	virtual int count(const string &mbName, int64 lbId = 0, int64 ubId = -1) = 0;
	
	virtual int addMsg(const string &mbName, const Message &msg) = 0;

	// get messages whose ids are not less than bMsgId(contains bMsgId itself)
	// bMsgId: the begin message id. 0 means the oldest message
	// length: amount of messages to get. -1 means to the latest message
	virtual int getMsgs(vector<Message> &vMsg, const string &mbName, int64 bMsgId = 0, 
		int length = -1, order_t orderBy = ORDER_BY_INCR) = 0;

	// get messages whose ids are not larger than bMsgId(contains eMsgId itself)
	// eMsgId: the end message id. -1 means the latest message
	// length: amount of messages to get. -1 means from the first message;
	virtual int getMsgsForward(vector<Message> &vMsg, const string &mbName, int64 eMsgId = -1, 
			int length = -1, order_t orderBy = ORDER_BY_DECR) = 0;

	virtual int delMsg(const string &mbName, int64 msgId) = 0;

	// delete messages whose ids are not less than bMsgId(contains bMsgId itself)
	// bMsgId: the begin message id. 0 means the oldest message
	// length: amount of message to delete. -1 means to the latest message
	virtual int delMsgsBackward(const string &mbName, int64 bMsgId = 0, int length = -1) = 0;

	// delete messages whose ids are not larger than eMsgId(contains eMsgId itself)
	// eMsgId: the end message id. -1 means the latest message
	// length: amount of message to delete. -1 means from the first message
	virtual int delMsgs(const string &mbName, int64 eMsgId = -1, int length = -1) = 0;

	virtual int clear(const string &mbName) = 0;

	virtual int incrId(const string &mbName, int64 &newId) = 0;

	virtual int getMsgId(const string &mbName, int64 &recentReadId) = 0;

	virtual int setMsgId(const string &mbName, int64 recentReadId) = 0;

};

class MsgDBFactory {
public:
	virtual MsgDB *newMsgDB(const Json::Value &config);
	virtual ~MsgDBFactory(){};
};

};
#endif /*__MSG_INTERFACE_H__ */
