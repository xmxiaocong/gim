#include "redis_msg_db.h"
#include <sstream>
#include "base/ef_utility.h"

namespace gim {

RedisMI::RedisMI(const Json::Value &config)
{
	m_cg = NULL;
	m_cfg = config;
	m_expiry = DEFAULT_MSG_EXPIRY_TIME;
	m_capacity = DEFAULT_MSG_BOX_CAPACITY;
	
	Json::Value etValue = m_cfg["ExpiryTime"];
	if (etValue.type() == Json::intValue || 
		etValue.type() == Json::uintValue) {
		m_expiry = etValue.asInt();
	}
	Json::Value cpValue = m_cfg["Capacity"];
	if (cpValue.type() == Json::intValue || 
		cpValue.type() == Json::uintValue) {
		m_capacity = cpValue.asInt();
	}
}
		
int RedisMI::bindCG(RedisCG *cg)
{
        if (m_cg == NULL) {
                m_cg = cg;
        }
        return 0;
}

int RedisMI::size(const string &mbName)
{
	int64 size = 0;
	DBHandle hndl = m_cg->getHndl(mbName);
	if (hndl == NULL) return CONNECT_CACHE_FAILED;
	hndl->ssetCard(mbName, size);
	return size;
}

int RedisMI::sizeFrom(const string &mbName, int64 bMsgId)
{
	int64 count = 0;
	DBHandle hndl = m_cg->getHndl(mbName);
	if (hndl == NULL) return CONNECT_CACHE_FAILED;
	hndl->ssetCount(mbName, bMsgId, DBL_MAX, count);
	return count;
}

int RedisMI::getMsgs(const string &mbName, int64 bMsgId, int length, vector<Message> &vMsg)
{
	if (length == 0) return -1;

	DBHandle hndl = m_cg->getHndl(mbName);
	if (hndl == NULL) return CONNECT_CACHE_FAILED;
	int ret = 0;

	vector<pair<string, string> > result, mems;
	vector<string> vmem;
	vector<pair<string, string> >::iterator it; 
	
	if (length == -1) {
		ret = hndl->ssetRangeByScoreWithScore(mbName, bMsgId, DBL_MAX, mems);
	} else {
		ret = hndl->ssetRangeByScoreWithScoreLimit(mbName, bMsgId, DBL_MAX, 0, length, mems);
	}
	
	if (ret < 0) return -1;
	
	for (it = mems.begin(); it != mems.end(); it++) {
		Message temp;
		temp.ParseFromString(it->second);
		if (!m_expiry || (gettime_ms() - temp.time() < m_expiry * 1000)) {
			vMsg.push_back(temp);
		}
	}
	
	return vMsg.size();
}

int RedisMI::getMsgsForward(const string &mbName, int64 bMsgId, int length, vector<Message> &vMsg)
{
	if (length == 0) return -1;

	DBHandle hndl = m_cg->getHndl(mbName);
	if (hndl == NULL) return CONNECT_CACHE_FAILED;
	int ret = 0;

	vector<pair<string, string> > result, mems;
	vector<string> vmem;
	vector<pair<string, string> >::reverse_iterator it; 

	if (length == -1) {
		ret = hndl->ssetRevRangeByScoreWithScore(mbName, bMsgId, DBL_MIN, mems);
	} else {
		ret = hndl->ssetRevRangeByScoreWithScoreLimit(mbName, bMsgId, DBL_MIN, 
			0, length, mems);
	}	
	
	if (ret < 0) return -1;
	
        for (it = mems.rbegin(); it != mems.rend(); it++) {
                Message temp;
                temp.ParseFromString(it->second);
                if (!m_expiry || (gettime_ms() - temp.time() < m_expiry * 1000)) {
                        vMsg.push_back(temp);
                }
        }

        return vMsg.size();
}

int RedisMI::addMsg(const string &mbName, Message &message)
{
        DBHandle hndl = m_cg->getHndl(mbName);
        if (hndl == NULL) return CONNECT_CACHE_FAILED;

        string str;
        stringstream ss;
        map<string, string> m;

        message.set_time(gettime_ms());
        ss << message.id();
        message.SerializeToString(&str);
        m.insert(pair<string, string>(ss.str(), str));
        int ret = hndl->ssetAdd(mbName, m);
        if (ret < 0) {
                return ret;
        }
	
	if (m_expiry > 0) {
	// first check and remove some messages if expired
		do {
			vector<pair<string, string> > mems;
        		hndl->ssetRangeWithScore(mbName, 0, 4, mems);
        		if (mems.size() <= 0) {
				break;
			}

        		vector<pair<string, string> >::reverse_iterator it = mems.rbegin();
        		Message tmp;
        		tmp.ParseFromString(it->second);
        		int64 now = gettime_ms();
        		if (now - tmp.time() > m_expiry * 1000) {
                		double exscore = atof(it->first.data());
                		hndl->ssetRemRangeByScore(mbName, DBL_MIN, exscore);
        		}
        	} while (0);
	}

        if (m_capacity > 0) {
                /* if message box is full, remove earlier messages*/
                hndl->ssetRemRangeByScore(mbName, DBL_MIN, message.id() - m_capacity);
        }
	
        /* update the expire time */
        if (m_expiry > 0) {
		hndl->keyExpire(mbName, m_expiry);
	}
	
        return ret;
}

int RedisMI::delMsg(const string &mbName, int64 msgId)
{
        DBHandle hndl = m_cg->getHndl(mbName);
        if (hndl == NULL) return CONNECT_CACHE_FAILED;

        int ret = hndl->ssetRemRangeByScore(mbName, msgId, msgId);
        if (ret >= 0 && m_expiry > 0) {
		hndl->keyExpire(mbName, m_expiry);
	}
	
        return ret;
}

int RedisMI::delMsgsBackward(const string &mbName, int64 bMsgId, int length)
{
	if (length == 0) return -1;

        DBHandle hndl = m_cg->getHndl(mbName);
        if (hndl == NULL) return CONNECT_CACHE_FAILED;

	int ret = 0;
	if (length == -1) {
		ret = hndl->ssetRemRangeByScore(mbName, bMsgId, DBL_MAX);
	} else {
		vector<string> vStr;
		ret = hndl->ssetRangeByScoreLimit(mbName, bMsgId, DBL_MAX, 0, length, vStr);
		if (ret < 0) return CONNECT_CACHE_FAILED;
		if (vStr.size() == 0) return -1;
		Message bMsg, eMsg;
		bMsg.ParseFromString(vStr[0]);
		eMsg.ParseFromString(vStr[vStr.size() - 1]);
        	ret = hndl->ssetRemRangeByScore(mbName, bMsg.id(), eMsg.id());
	}
	
        if(ret >= 0 && m_expiry > 0) {
		hndl->keyExpire(mbName, m_expiry);
	}

        return ret;
}

int RedisMI::delMsgs(const string &mbName, int64 bMsgId, int length)
{
	if (length == 0) return -1;
	
        DBHandle hndl = m_cg->getHndl(mbName);
        if (hndl == NULL) return CONNECT_CACHE_FAILED;

	int ret = 0;
	if (length == -1) {
		ret = hndl->ssetRemRangeByScore(mbName, DBL_MIN, bMsgId);
	} else {
		vector<string> vStr;
		ret = hndl->ssetRevRangeByScoreLimit(mbName, bMsgId, DBL_MIN, 0, length, vStr);
		if (ret < 0) return CONNECT_CACHE_FAILED;
		if (vStr.size() == 0) return -1;
		Message bMsg, eMsg;
		bMsg.ParseFromString(vStr[0]);
		eMsg.ParseFromString(vStr[vStr.size() - 1]);
        	ret = hndl->ssetRemRangeByScore(mbName, eMsg.id(), bMsg.id());
	}

        if(ret >= 0 && m_expiry > 0) {
		hndl->keyExpire(mbName, m_expiry);
	}
	
        return ret;
}

int RedisMI::clear(const string &mbName)
{
        DBHandle hndl = m_cg->getHndl(mbName);
        if (hndl == NULL) return CONNECT_CACHE_FAILED;

        int ret = hndl->ssetRemRange(mbName, 0, -1);
        if (ret >= 0 && m_expiry > 0) {
		hndl->keyExpire(mbName, m_expiry);
	}

        return ret;
}

int RedisMI::getMsgId(const string &mbName, int64 &recentReadId)
{
        DBHandle hndl = m_cg->getHndl(mbName);
        if (hndl == NULL) return CONNECT_CACHE_FAILED;

        string str;
        int ret = hndl->strGet(mbName, str);
        if (ret < 0) return ret;
        recentReadId = atoll(str.data());

        return ret;
}

int RedisMI::setMsgId(const string &mbName, int64 recentReadId)
{
        DBHandle hndl = m_cg->getHndl(mbName);
        if (hndl == NULL) return CONNECT_CACHE_FAILED;

        stringstream ss;
        ss << recentReadId;
        return hndl->strSet(mbName, ss.str());
}

int RedisMI::incrId(const string &mbName, int64 &newId)
{
        DBHandle hndl = m_cg->getHndl(mbName);
        if (hndl == NULL) return CONNECT_CACHE_FAILED;

        return hndl->strIncrBy(mbName, 1, newId);
}

};                                                                                                   

