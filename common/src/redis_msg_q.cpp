#include "redis_msg_q.h"
#include "redis_cg.h"
#include <map>
#include "base/ef_utility.h"
#include <sstream>

namespace gim{

#define  HANDLE_CHECK(hash) \
	if(!m_cg)return CACHE_NOT_EXIST; \
	DBHandle handle = m_cg->getHndl(hash); \
	if(!handle)return CONNECT_CACHE_FAILED

	RedisMQ::RedisMQ(const Json::Value& config){
		m_expiry = DEFAULT_EXPIRE;
		m_capacity = DEFAULT_CAPACITY;

		Json::Value etValue = config["ExpiryTime"];
		if (etValue.type() == Json::intValue || 
			etValue.type() == Json::uintValue) {
				m_expiry = etValue.asInt();
		}
		Json::Value cpValue = config["Capacity"];
		if (cpValue.type() == Json::intValue || 
			cpValue.type() == Json::uintValue) {
				m_capacity = cpValue.asInt();
		}

		m_cg = new RedisCG(config);
	}
	RedisMQ::~RedisMQ(){
		if (m_cg) {
			delete m_cg;
		}
	}
	int RedisMQ::incrId(const string& hash, const string& key, int64& id){
		HANDLE_CHECK(hash);
		return handle->strIncr(key, id);
	}

	int RedisMQ::getId(const string& hash, const string& key, int64& id){
		HANDLE_CHECK(hash);
		string value;
		int ret = handle->strIncrBy(key, 0, id);
		return ret;
	}
	int64 RedisMQ::size(const string& hash, const string& key){
		HANDLE_CHECK(hash);
		int64 s;
		int ret = handle->ssetCard(key, s);
		return (ret>=0)?s:-1;
	}

	int RedisMQ::add(const string& hash, const string& key, const Message& msg){	
		HANDLE_CHECK(hash);

		string s;
		msg.SerializeToString(&s);

		map<string, string> m;
		m.insert(pair<string, string>(ef::itostr(msg.id()), s));
		int ret = handle->ssetAdd(key, m);
		if(ret < 0)
			return ret;

		if(m_capacity>0){
			int64 size=0;
			if(handle->ssetCard(key, size) >= 0
				&&size > m_capacity){
					handle->ssetRemRange(key, 0, size-m_capacity);
			}
		}

		if(m_expiry){
			handle->keyExpire(key, m_expiry);
		}
		return 0;
	}

	int RedisMQ::del(const string& hash, const string& key, int64 start, int64 end){
		HANDLE_CHECK(hash);
		return handle->ssetRemRangeByScore(key, start, end);
	}

	int RedisMQ::get(const string& hash, const string& key, int64 start, int len, vector<Message>& msgs){
		if(len <= 0 || start < 0)
			return -1;

		HANDLE_CHECK(hash);
		double ds = start <= 0 ? DBL_MIN : start;
		int64 now = gettime_ms();

		const int loopmax = 5;
		int loopcount=0;
		do 
		{
			++loopcount;
			int left = len-msgs.size();
			vector<pair<string, string> >  vs;
			int ret = handle->ssetRangeByScoreWithScoreLimit(key, ds, DBL_MAX, 0, left, vs);
			if(ret < 0)
				return ret;

			for (unsigned int n=0;n<vs.size();++n){
				Message m;
				m.ParseFromString(vs[n].second);
				ds = m.id();

				if(m.time()+m.expire() < now)
					continue;

				msgs.push_back(m);
			}

			if (left > vs.size() || len == msgs.size())
				break;
		} while (loopcount < loopmax);

		return 0;
	}
}