#ifndef _JSON_REDIS_DB_H_
#define _JSON_REDIS_DB_H_
#include <json/json.h>
#include <string>
#include <vector>
#include "base/ef_btype.h"
#include "message.pb.h"

using namespace std;
using namespace ef;

namespace gim{
	class RedisCG;
	class RedisMQ{
	public:
		enum{
			DEFAULT_EXPIRE = 60 * 60 * 24 * 7,
			DEFAULT_CAPACITY = 100
		};

		RedisMQ(const Json::Value& config);
		~RedisMQ();
		int incrId(const string& hash, const string& key, int64& id);
		int getId(const string& hash, const string& key, int64& id);
		int64 size(const string& hash, const string& key);
		int del(const string& hash, const string& key, int64 start, int64 end);

		int add(const string& hash, const string& key, const Message& m);
		int get(const string& hash, const string& key, int64 start, int len, vector<Message>& msgs);
	private:
		RedisCG *m_cg;
		int m_expiry;
		int m_capacity;
	};
}
#endif //_JSON_REDIS_DB_H_
