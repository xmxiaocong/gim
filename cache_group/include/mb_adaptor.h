#ifndef __MB_ADAPTOR_H__
#define __MB_ADAPTOR_H__

#include <string>
#include <vector>
#include "json/json.h"
#include "base/ef_btype.h"
#include "message.pb.h"
#include "cache_group.h"

namespace gim {

using namespace std;
using namespace ef;

#define DEFAULT_MB_SIZE 100
#define DEFAULT_MSG_EXPIRY_TIME  60 * 60 * 24 * 7 /* a week */

class MbAdaptor {
public:
	MbAdaptor(int capability = DEFAULT_MB_SIZE, 
		int64 expiry = DEFAULT_MSG_EXPIRY_TIME):
		m_capability(capability),m_expiry(expiry){};
	virtual ~MbAdaptor(){};
	void setCapability(int capability){m_capability = capability;};
	void setExpiry(int64 expiry){m_expiry = expiry;};

	virtual int rebindCG(const CacheGroup *cg) = 0;
	virtual int size(const string &mbName) = 0;
	virtual int sizeFrom(const string &mbName, int64 bMsgId) = 0;
	virtual int addMsg(const string &mbName, Message &msg) = 0;
	virtual int getMsgs(const string &mbName, 
			int64 bMsgId, int length, vector<Message> &msg) = 0;
	virtual int getMsgsBackward(const string &mbName, 
			int64 bMsgId, int length, vector<Message> &msg) = 0;
	virtual int delMsg(const string &mbName, int64 msgId) = 0;
	virtual int delMsgs(const string &mbName, int64 bMsgId) = 0;
	virtual int delMsgsBackward(const string &mbName, int64 bMsgId) = 0;
	virtual int clear(const string &mbName) = 0;
	virtual int getMsgId(const string &mbName, int64 &recentReadId) = 0;
	virtual int setMsgId(const string &mbName, int64 recentReadId) = 0;
	virtual int incrId(const string &mbName, int64 &newId) = 0;
	virtual int clearExpiredMessage(const string &mbName) = 0;
protected:
	int m_capability;
	int64 m_expiry;
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
