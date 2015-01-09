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

class MsgDB {
public:
	virtual ~MsgDB(){};

	virtual int size(const string &mbName) = 0;

	virtual int addMsg(const string &mbName, Message &msg) = 0;

	virtual int getMsgsForward(const string &mbName, 
		int64 beginMsgId, int length, vector<Message> &msg) = 0;

	virtual int getMsgs(const string &mbName, 
		int64 beginMsgId, int length, vector<Message> &msg) = 0;

	virtual int delMsg(const string &mbName, int64 msgId) = 0;

	virtual int delMsgsBackward(const string &mbName,
		int64 beginMsgId, int length) = 0;

	virtual int delMsgs(const string &mbName, 
		int64 beginMsgId, int length) = 0;
	
	virtual int clear(const string &mbName) = 0;

	virtual int incrId(const string &key, int64 &newId) = 0;
	
	virtual int getMsgId(const string &key, int64 &curId) = 0;

	virtual int setMsgId(const string &key, int64 id) = 0;
};

class MsgDBFactory {
public:
	virtual MsgDB *newMsgDB(const Json::Value &config);
	virtual ~MsgDBFactory(){};
};

};
#endif /*__MSG_INTERFACE_H__ */
