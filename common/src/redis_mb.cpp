#include "redis_mb.h"
#include <sstream>
#include "base/ef_utility.h"

namespace gim {

RedisMb::RedisMb(const Json::Value &config)
{
	m_cg = NULL;
	m_cfg = config;
	m_expiry = DEFAULT_MSG_EXPIRY_TIME;
	m_capability = DEFAULT_MB_CAPABILITY;
	
	Json::Value etValue = m_cfg["ExpiryTime"];
	if (etValue.type() == Json::intValue || 
		etValue.type() == Json::uintValue) {
		m_expiry = etValue.asInt();
	}
	Json::Value cpValue = m_cfg["Capability"];
	if (cpValue.type() == Json::intValue || 
		cpValue.type() == Json::uintValue) {
		m_capability = cpValue.asInt();
	}
}
		
int RedisMb::bindCG(RedisCG *cg)
{
        if (m_cg == NULL) {
                m_cg = cg;
        }
        return 0;
}

int RedisMb::size(const string &mbName)
{
	int64 size = 0;
	DBHandle hndl = m_cg->getHndl(mbName);
	if (hndl == NULL) return CONNECT_CACHE_FAILED;
	hndl->ssetCard(mbName, size);
	return size;
}

int RedisMb::sizeFrom(const string &mbName, int64 bMsgId)
{
	int64 count = 0;
	DBHandle hndl = m_cg->getHndl(mbName);
	if (hndl == NULL) return CONNECT_CACHE_FAILED;
	hndl->ssetCount(mbName, bMsgId, DBL_MAX, count);
	return count;
}

int RedisMb::getMsgs(const string &mbName, int64 bMsgId, int length, vector<Message> &vMsg)
{
	if (length == 0) return -1;

	DBHandle hndl = m_cg->getHndl(mbName);
	if (hndl == NULL) return CONNECT_CACHE_FAILED;
	int cnt = 0;

	vector<pair<string, string> > result, mems;
	vector<string> vmem;
	vector<pair<string, string> >::iterator it; 
	
	hndl->ssetRangeByScoreWithScoreLimit(mbName, bMsgId, DBL_MAX, 0, length, mems);
	for (it = mems.begin(); it != mems.end(); it++) {
		Message temp;
		temp.ParseFromString(it->second);
		if (!m_expiry || (gettime_ms() - temp.time() < m_expiry * 1000)) {
			vMsg.push_back(temp);
			cnt++;
		}
	}
	
	return cnt;
}

int RedisMb::getMsgsForward(const string &mbName, int64 bMsgId, int length, vector<Message> &vMsg)
{
	if (length == 0) return -1;

	DBHandle hndl = m_cg->getHndl(mbName);
	if (hndl == NULL) return CONNECT_CACHE_FAILED;
	int ret = 0;

	vector<pair<string, string> > result, mems;
	vector<string> vmem;
	vector<pair<string, string> >::iterator it; 

	int64 cnt;
	hndl->ssetCount(mbName, DBL_MIN, bMsgId, cnt);
	if (cnt <= 0) {
		return 0;
	} else if (cnt <= length) {
		hndl->ssetRangeByScoreWithScoreLimit(mbName, DBL_MIN, bMsgId, 0, length, mems);
	} else {
		hndl->ssetRangeByScoreWithScoreLimit(mbName, DBL_MIN, bMsgId, 
			cnt - length, length, mems);
	}	
	
        for (it = mems.begin(); it != mems.end(); it++) {
                Message temp;
                temp.ParseFromString(it->second);
                if (!m_expiry || (gettime_ms() - temp.time() < m_expiry * 1000)) {
                        vMsg.push_back(temp);
                        ret++;
                }
        }

        return ret;
}

int RedisMb::addMsg(const string &mbName, Message &message)
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
                clearExpiredMessage(mbName);
        }

        if (m_capability > 0) {
                /* if message box is full, clear */
                hndl->ssetRemRangeByScore(mbName, DBL_MIN, message.id() - m_capability);
        }
        /* update the expire time */
        if (m_expiry > 0) {
		hndl->keyExpire(mbName, m_expiry);
	}
	
        return ret;
}

int RedisMb::delMsg(const string &mbName, int64 msgId)
{
        DBHandle hndl = m_cg->getHndl(mbName);
        if (hndl == NULL) return CONNECT_CACHE_FAILED;

        int ret = hndl->ssetRemRangeByScore(mbName, msgId, msgId);
        if (ret >= 0 && m_expiry > 0) {
		hndl->keyExpire(mbName, m_expiry);
	}
	
        return ret;
}

int RedisMb::delMsgsBackward(const string &mbName, int64 bMsgId, int length)
{
	if (length == 0) return -1;

        DBHandle hndl = m_cg->getHndl(mbName);
        if (hndl == NULL) return CONNECT_CACHE_FAILED;

	double endId;
	if (length == -1) {
		endId = DBL_MAX;
	} else {
		endId = bMsgId + length - 1;
	}
        int ret = hndl->ssetRemRangeByScore(mbName, bMsgId, endId);
        if(ret >= 0 && m_expiry > 0) {
		hndl->keyExpire(mbName, m_expiry);
	}

        return ret;
}

int RedisMb::delMsgs(const string &mbName, int64 bMsgId, int length)
{
	if (length == 0) return -1;
	
        DBHandle hndl = m_cg->getHndl(mbName);
        if (hndl == NULL) return CONNECT_CACHE_FAILED;

	double minId;
	if (length == -1) {
		minId = DBL_MIN;
	} else {
		minId = bMsgId - length + 1;
	}
	
        int ret = hndl->ssetRemRangeByScore(mbName, minId, bMsgId);
        if(ret >= 0 && m_expiry > 0) {
		hndl->keyExpire(mbName, m_expiry);
	}
	
        return ret;
}

/* clear expired messages */
int RedisMb::clearExpiredMessage(const string &mbName)
{
        DBHandle hndl = m_cg->getHndl(mbName);
        if (hndl == NULL) return CONNECT_CACHE_FAILED;

        int64 now = gettime_ms();
        vector<pair<string, string> > mems;

        hndl->ssetRangeWithScore(mbName, 0, 4, mems);
        if (mems.size() <= 0) return 0;

        vector<pair<string, string> >::iterator it = mems.end() - 1;
        Message tmp;
        tmp.ParseFromString(it->second);
        if (now - tmp.time() > m_expiry * 1000) {
                double exscore = atof(it->first.data());
                return hndl->ssetRemRangeByScore(mbName, DBL_MIN, exscore);
        }

        return 0;
}

int RedisMb::clear(const string &mbName)
{
        DBHandle hndl = m_cg->getHndl(mbName);
        if (hndl == NULL) return CONNECT_CACHE_FAILED;

        int ret = hndl->ssetRemRangeByRank(mbName, 0, -1);
        if (ret >= 0 && m_expiry > 0) {
		hndl->keyExpire(mbName, m_expiry);
	}

        return ret;
}

int RedisMb::getMsgId(const string &mbName, int64 &recentReadId)
{
        DBHandle hndl = m_cg->getHndl(mbName);
        if (hndl == NULL) return CONNECT_CACHE_FAILED;

        string str;
        int ret = hndl->strGet(mbName, str);
        if (ret < 0) return ret;
        recentReadId = atoll(str.data());

        return ret;
}

int RedisMb::setMsgId(const string &mbName, int64 recentReadId)
{
        DBHandle hndl = m_cg->getHndl(mbName);
        if (hndl == NULL) return CONNECT_CACHE_FAILED;

        stringstream ss;
        ss << recentReadId;
        return hndl->strSet(mbName, ss.str());
}

int RedisMb::incrId(const string &mbName, int64 &newId)
{
        DBHandle hndl = m_cg->getHndl(mbName);
        if (hndl == NULL) return CONNECT_CACHE_FAILED;

        return hndl->strIncrBy(mbName, 1, newId);
}

};                                                                                                   

