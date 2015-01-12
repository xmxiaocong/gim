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
	
	// returns message numbers between lbId and ubId
	// lbId: lower bound. 0 means the first message
	// ubId: upper bound. -1 means the latest message
	int count(const string &mbName, int64 lbId = 0, int64 ubId = -1);

	int addMsg(const string &mbName, Message &msg);

	// get messages whose id are not less than bMsgId(contains bMsgId itself)
	int getMsgs(const string &mbName, int64 bMsgId, int length,
			vector<Message> &msg);

	// get messages whose id are not larger than bMsgId(contains bMsgId itself)
	int getMsgsForward(const string &mbName, 
			int64 bMsgId, int length, vector<Message> &msg);

	int delMsg(const string &mbName, int64 msgId);

	// delete messages whose id are not less than bMsgId(contains bMsgId itself)
	int delMsgsBackward(const string &mbName, int64 bMsgId, int length = -1);

	// delete messages whose id are not larger than bMsgId(contains bMsgId itself)
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

}
	

#endif //__REDIS_ADAPTOR_H__
