#ifndef __REDIS_ADAPTOR_H__
#define __REDIS_ADAPTOR_H__

#include "mb_adaptor.h"
#include "redis_cg.h"

namespace gim {

using namespace std;
using namespace ef;

class RedisMb : public MbAdaptor {
public:
	RedisMb():m_cg(NULL){};
	int rebindCG(const CacheGroup *cg);
	int size(const string &mbName);
	int sizeFrom(const string &mbName, int64 bMsgId);
	int addMsg(const string &mbName, Message &msg);
	int getMsgs(const string &mbName, int64 bMsgId, int length,
			vector<Message> &msg);
	int getMsgsBackward(const string &mbName, 
			int64 bMsgId, int length, vector<Message> &msg);
	int delMsg(const string &mbName, int64 msgId);
	int delMsgs(const string &mbName, int64 bMsgId);
	int delMsgsBackward(const string &mbName, int64 bMsgId);
	int clear(const string &mbName);
	int getMsgId(const string &mbName, int64 &recentReadId);
	int setMsgId(const string &mbName, int64 recentReadId);
	int incrId(const string &mbName, int64 &newId);
	int clearExpiredMessage(const string &mbName);
private:
	RedisCG *m_cg;
};

class RdsMbFactory : public MbAdptFactory {
public:
	MbAdaptor *createNewMbAdpt(const Json::Value &config)
	{
		return new RedisMb();
	}
};

}
	

#endif //__REDIS_ADAPTOR_H__
