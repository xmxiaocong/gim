#ifndef __MB_ADAPTOR_H__
#define __MB_ADAPTOR_H__

#include <string>
#include <vector>
#include "json/json.h"
#include "base/ef_btype.h"
#include "message.pb.h"

namespace gim {

using namespace std;
using namespace ef;

#define DEFAULT_MB_SIZE 100
#define DEFAULT_MSG_EXPIRY_TIME  60 * 60 * 24 * 7 /* a week */

class MbAdaptor {
public:
	virtual ~MbAdaptor(){};

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
};

class MbAdptFactory {
public:
	virtual MbAdaptor *createNewMbAdpt(const Json::Value &config)
	{
		return NULL;
	}
};

};
#endif /*__MB_ADAPTOR_H__ */
