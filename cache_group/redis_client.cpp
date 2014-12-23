#include <sstream>
#include <string>
#include <stdlib.h>
#include "redis_client.h"

namespace gim {

static int _getResultFromReply(redisReply *reply, string &result)
{
	if(!reply->str) return CACHE_NOT_EXIST;
	result.append(reply->str, reply->len);
	return 0;
}

static int _getResultFromReply(redisReply *reply, vector<string> &result)
{
	if (reply->type != REDIS_REPLY_ARRAY) return CACHE_NOT_EXIST;
	for (unsigned int i = 0; i < reply->elements; i++) { 
		string s(reply->element[i]->str, reply->element[i]->len);
		result.push_back(s);
	}
	return 0;
}

static int _getResultFromReply(redisReply *reply, map<string, string> &result)
{
	if (reply->type != REDIS_REPLY_ARRAY) return CACHE_NOT_EXIST;
	for (unsigned int i = 0; i < reply->elements; i+=2) {
		string k(reply->element[i]->str, reply->element[i]->len);
		string v(reply->element[i+1]->str, reply->element[i+1]->len);
		result[k] = v;
	}
	return 0;
}

static int _getResultFromReply(redisReply *reply, vector<pair<string, string> > &result)
{
	if (reply->type != REDIS_REPLY_ARRAY) return CACHE_NOT_EXIST;
	for (unsigned int i = 0; i < reply->elements; i+=2) { 
		string f(reply->element[i+1]->str, reply->element[i+1]->len);
		string s(reply->element[i]->str, reply->element[i]->len);
		result.push_back(pair<string, string>(f, s));
	}
	return 0;
}

static int _getResultFromReply(redisReply *reply, int64 &result)
{
	if (reply->type != REDIS_REPLY_INTEGER) return CACHE_NOT_EXIST;
	result = reply->integer;
	return 0;
}

static int _getResultFromReply(redisReply *reply, int &result)
{
	if (reply->type != REDIS_REPLY_INTEGER) return CACHE_NOT_EXIST;
	result = reply->integer;
	return 0;
}

static int _getResultFromReply(redisReply *reply, float &result)
{
	if (!reply->str) return CACHE_NOT_EXIST;
	string tmp;
	tmp.append(reply->str, reply->len);
	result = atof(tmp.data());
	return 0;
}

template <typename T> 
int getResultFromReply(redisReply *reply, T &result)
{
	if(reply == NULL || reply->type == REDIS_REPLY_NIL){
		return CACHE_NOT_EXIST;
	} else if (reply->type == REDIS_REPLY_ERROR) { 
		return -1;
	}
	return _getResultFromReply(reply, result);
}

int RedisCli::connect(const string &addr, int port, const string &passWd)
{
	struct timeval timeout = { 1, 500000 };
	int ret = 0;
	stringstream os;

	m_addr = addr;
	m_port = port;
	m_passWd = passWd;
	
	m_c = redisConnectWithTimeout((char*)addr.data(), port, timeout);
	os << addr << ":" << port;
	if (!m_c) {
		os << "    CONNECT failed"; 
		ret = CONNECT_CACHE_FAILED;
		goto exit;
	}
	if (m_c->err) {
		os << "    CONNECT failed, reason:" << m_c->err;
		redisFree(m_c);
		m_c = NULL; 
		ret = CONNECT_CACHE_FAILED;
		goto exit;
	}
	os << "    CONNECT success";
exit:
	if (m_cmdLog)
		m_cmdLog(os.str());
	if (m_c && passWd.size()) {
		//when auth,the m_c is allways not null,
		//so _doexecmd->reconnect->connauth->_doexecmd
		//will not loop
		ret = connAuth(passWd);
	}
	return ret;
}

void RedisCli::addLog(const string &data)
{
	if (m_cmdLog) {
		stringstream ss;
		ss << m_addr << ":" << m_port << "    " << data;
		m_cmdLog(ss.str());
	}
}

int RedisCli::reconnect()
{
	disconnect();
	addLog("reconnect");
	return connect(m_addr, m_port, m_passWd);
}

int RedisCli::disconnect()
{
	if(m_c){
		if (!m_c->err) connQuit();
		redisFree(m_c);
		m_c = NULL;
	}
	return	0;
}

int64 RedisCli::_exeCmd(Replyer &rpler, const string &cmd){
	int l = m_retry;
	int64 ret = 0;
	while(l-- > 0){
		ret = _doExeCmd(rpler, cmd);
		if(ret >= 0 || ret == -4)
			return ret;
	}
	return ret;
}

int64 RedisCli::_doExeCmdNoReconnect(Replyer &rpler, const string &cmd)
{
	redisReply *tmpReply = (redisReply *)redisCommand(m_c, cmd.data());
	if (m_cmdLog) {
		stringstream ss;
		ss << m_addr << ":" << m_port << "    " << cmd;
		if (tmpReply == NULL || m_c->err) {
			ss << "    failed";
		} else if (tmpReply->type == REDIS_REPLY_ERROR) {
			ss << "    failed, reason:" << tmpReply->str;
		} else {
			ss << "    success";
		}
		m_cmdLog(ss.str());
	}
	if (tmpReply == NULL) {
		disconnect();
		return -1;
	}
	if (m_c->err) {
		disconnect();
		return -3;
	}
	//logic error, do not disconnect it!
	if (tmpReply->type == REDIS_REPLY_ERROR) {
		freeReplyObject(tmpReply);
		return -4;
	}
	rpler.setReply(tmpReply);
	return rpler.reply()->integer;

}

int64 RedisCli::_doExeCmd(Replyer &rpler, const string &cmd)
{
	int64 ret = 0;
	if(!m_c){
		ret = reconnect();
	}	
	if(ret < 0){
		return  ret; 
	}
	return _doExeCmdNoReconnect(rpler, cmd);	
}

int64 RedisCli::_exeCmdWithNoOutput(const string &cmd)
{
	Replyer rpler;
	return _exeCmd(rpler, cmd);
}

template <typename T>
int64 RedisCli::_exeCmdWithOutput(const string &cmd, T &output)
{
	int64 ret = 0;
	Replyer rpler;

	if ((ret = _exeCmd(rpler, cmd)) < 0) 
		return -1;
	ret = getResultFromReply(rpler.reply(), output);
	return ret;
}

int RedisCli::keyDel(const string &key)
{
	return _exeCmdWithNoOutput("DEL " + key);
}

int RedisCli::keyExists(const string &key)
{
	return _exeCmdWithNoOutput("EXISTS " + key);
}

int RedisCli::keyExpire(const string &key, int seconds)
{
	stringstream ss;
	ss << "EXPIRE " << key << " " << seconds;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::strGet(const string &key, string &value)
{
	return _exeCmdWithOutput("GET " + key, value);
}

int RedisCli::strGetSet(const string &key, const string &value, string &oldvalue)
{
	return _exeCmdWithOutput("GETSET " + key + " " + value, oldvalue);
}

int64 RedisCli::strIncr(const string &key, int64 &afterIncr)
{
	return _exeCmdWithOutput("INCR " + key, afterIncr);
}

int64 RedisCli::strIncrBy(const string &key, int64 increment, int64 &afterIncr)
{
	stringstream ss;
	ss << "INCRBY " << key << " " << increment;
	return _exeCmdWithOutput(ss.str(), afterIncr);
}

int RedisCli::strSet(const string &key, const string &value, const string &options)
{
	return _exeCmdWithNoOutput("SET " + key + " " + value + " " + options);
}

int RedisCli::hashDel(const string &key, const string &field)
{
	return _exeCmdWithNoOutput("HDEL " + key + " " + field);
}

int RedisCli::hashExists(const string &key, const string &field)
{
	return _exeCmdWithNoOutput("HEXISTS " + key + " " + field);
}

int RedisCli::hashGet(const string &key, const string &field, string &value)
{
	return _exeCmdWithOutput("HGET " + key + " " + field, value);
}

int RedisCli::hashGetAll(const string &key, map<string, string> &mfv)
{
	return _exeCmdWithOutput("HGETALL " + key, mfv);
}

int RedisCli::hashMSet(const string &key, const map<string, string> &mfv)
{
	stringstream ss;
	ss << "HMSET " << key << " ";
	map<string, string>::const_iterator it;
	for (it = mfv.begin(); it != mfv.end(); it++) {
		ss << it->first << " " << it->second << " ";
	}
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::hashSet(const string &key, const string &field, const string &value)
{
	return _exeCmdWithNoOutput("HSET " + key + " " + field + " " + value);
}

/* set related */
int RedisCli::setAdd(const string &key, const vector<string> &members)
{
	stringstream ss;
	ss << "SADD " << key << " ";
	vector<string>::const_iterator it;
	for (it = members.begin(); it != members.end(); it++) {
		ss << *it << " ";
	}
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::setIsMember(const string &key, const string &member, int &ismember)
{
	return _exeCmdWithOutput("SISMEMBER " + key + " " + member, ismember);
}	

int RedisCli::setMembers(const string &key, vector<string> &members)
{
	return _exeCmdWithOutput("SMEMBERS " + key, members);
}

int RedisCli::setRem(const string &key, const vector<string> &members)
{
	stringstream ss;
	ss << "SREM " << key << " ";
	vector<string>::const_iterator it;
	for (it = members.begin(); it != members.end(); it++) {
		ss << *it << " ";
	}
	return _exeCmdWithNoOutput(ss.str());
}

/* sorted set related */
int RedisCli::ssetAdd(const string &key, const map<string, string> &members)
{
	stringstream ss;
	ss << "ZADD " << key << " ";
	map<string, string>::const_iterator it;
	for (it = members.begin(); it != members.end(); it++) {
		ss << it->first << " " << it->second << " ";
	}
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::ssetCard(const string &key)
{
	return _exeCmdWithNoOutput("ZCARD " + key);
}

int RedisCli::ssetCount(const string &key, double minscore, double maxscore)
{
	stringstream ss;
	ss << "ZCOUNT " << key << " ";
	if (minscore == DBL_MIN) {
		ss << "-inf ";
	} else {
		ss << minscore << " ";
	} 
	if (maxscore == DBL_MAX) {
		ss << "+inf";
	} else {
		ss << maxscore;
	}
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::ssetRange(const string &key, int start, int stop, vector<string> &members)
{
	stringstream ss;
	ss << "ZRANGE " << key << " " << start << " " << stop;
	return _exeCmdWithOutput(ss.str(), members);
}

int RedisCli::ssetRangeWithScore(const string &key, int start, int stop, vector<pair<string, string> > &members)
{
	stringstream ss;
	ss << "ZRANGE " << key << " " << start << " " << stop << " WITHSCORES";
	return _exeCmdWithOutput(ss.str(), members);
}

int RedisCli::ssetRangeByScoreWithScore(const string &key, double minscore, double maxscore, 
		const string &options, vector<pair<string, string> > &members)
{
	stringstream ss;
	
	ss << "ZRANGEBYSCORE " << key << " ";
	if (minscore == DBL_MIN) {
		ss << "-inf ";
	} else {
		ss << minscore << " ";
	}

	if (maxscore == DBL_MAX) {
		ss << "+inf ";
	} else {
		ss << maxscore << " ";
	}
	
	ss << "WITHSCORES " << options;
	return _exeCmdWithOutput(ss.str(), members);
}

int RedisCli::ssetRemRangeByRank(const string &key, int start, int stop)
{
	stringstream ss;
	ss << "ZREMRANGEBYRANK " << key << " " << start << " " << stop;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::ssetRemRangeByScore(const string &key, double minscore, double maxscore)
{
	stringstream ss;

	ss << "ZREMRANGEBYSCORE " << key << " ";
	if (minscore == DBL_MIN) {
		ss << "-inf ";
	} else {
		ss << minscore << " ";
	}
	if (maxscore == DBL_MAX) {
		ss << "+inf";
	} else {
		ss << maxscore;
	} 
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::ssetRevRange(const string &key, int start, int stop, vector<string> &members)
{
	stringstream ss;
	ss << "ZREVRANGE " << key << " " << start << " " << stop;
	return _exeCmdWithOutput(ss.str(), members);
}

/* connection related */
int RedisCli::connAuth(const string &password)
{
	stringstream ss;
	Replyer rpler;
	ss << "AUTH " << password;
	return _doExeCmdNoReconnect(rpler, ss.str());
}

int RedisCli::connPing()
{
	return _exeCmdWithNoOutput("PING");
}

int RedisCli::connQuit()
{
	return _exeCmdWithNoOutput("QUIT");
}

int RedisCli::servBGSave()
{
	return _exeCmdWithNoOutput("BGSAVE");
}

int RedisCli::servCfgGet(const string &parameter, vector<string> &results)
{
	return _exeCmdWithOutput("CONFIG GET " + parameter, results);
}

int RedisCli::servCfgSet(const string &parameter, const string &value)
{
	return _exeCmdWithNoOutput("CONFIG SET " + parameter + " " + value);
}

int RedisCli::servDBSize()
{
	return _exeCmdWithNoOutput("DBSIZE");
}

int RedisCli::servSlaveOf(const string &masterip, int masterport)
{
	stringstream ss;
	ss << "SLAVEOF " << masterip << " " << masterport;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::servSlaveOfNoOne()
{
	return _exeCmdWithNoOutput("SLAVEOF NO ONE");
}

#ifdef ENABLE_UNSTABLE_INTERFACE

int RedisCli::keyDump(const string &key, string &value)
{
	return _exeCmdWithOutput("DUMP " + key, value);
}

int RedisCli::keyExpireAt(const string &key, int64 timeStamp)
{
	stringstream ss;
	ss << "EXPIREAT " << key << " " << timeStamp;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::keyKeys(const string &pattern, vector<string> &keys)
{
	return _exeCmdWithOutput("KEYS " + pattern, keys);
}

int RedisCli::keyMigrate(const string &host, int port, const string &key, 
  		int destDB, int timeout, const string &options)
{
	stringstream ss;
	ss << "MIGRATE " << host << " " << port << " " << key << " " 
		<< destDB << " " << timeout << " " << options;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::keyMove(const string &key, int destDB)
{
	stringstream ss;
	ss << "MOVE " << key << " " << destDB;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::keyObject(const string &subCmd, const string &key, vector<string> &result)
{
	return _exeCmdWithOutput("OBJECT " + subCmd + " " + key, result);
}

int RedisCli::keyPersist(const string &key)
{
	return _exeCmdWithNoOutput("PERSIST " + key);
}

int RedisCli::keyPExpire(const string &key, int64 milliSeconds)
{
	stringstream ss;
	ss << "PEXPIRE " << key << " " << milliSeconds;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::keyPExpireAt(const string &key, int64 milliTimeStamp)
{
	stringstream ss;
	ss << "PEXPIREAT " << key << " " << milliTimeStamp;
	return _exeCmdWithNoOutput(ss.str());
}

int64 RedisCli::keyPTTL(const string &key, int64 &milliTTL)
{
	return _exeCmdWithOutput("PTTL " + key, milliTTL);
}

int RedisCli::keyRandomKey(string &key)
{
	return _exeCmdWithOutput("RANDOMKEY", key);
}

int RedisCli::keyRename(const string &key, const string &newKey)
{
	return _exeCmdWithNoOutput("RENAME " + key + " " + newKey);
}

int RedisCli::keyRenameNx(const string &key, const string &newKey)
{
	return _exeCmdWithNoOutput("RENAMENX " + key + " " + newKey);
}

int RedisCli::keyRestore(const string &key, int ttl, const string &value, const string &option)
{
	stringstream ss;
	ss << "RESTORE " << key << " " << ttl << " " << value << " " << option;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::keyTTL(const string &key, int &ttl)
{
	return _exeCmdWithOutput("TTL " + key, ttl);
}

int RedisCli::keyType(const string &key, string &type)
{
	return _exeCmdWithOutput("TYPE " + key, type);
}

int RedisCli::strAppend(const string &key, const string &value)
{
	return _exeCmdWithNoOutput("APPEND " + key + " " + value);
}

int RedisCli::strBitCount(const string &key, int start, int end)
{
	stringstream ss;
	ss << "BITCOUNT " << key << " " << start << " " << end;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::strBitOp(const string &op, const string &destKey, const vector<string> &keys)
{
	stringstream ss;
	ss << "BITOP " << op << " " << destKey << " ";
	vector<string>::const_iterator it;
	for (it = keys.begin(); it != keys.end(); it++) {
		ss << *it << " ";
	}
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::strDecr(const string &key, int &afterDecr)
{
	return _exeCmdWithOutput("DECR " + key, afterDecr);
}

int RedisCli::strDecrBy(const string &key, int decrement, int &afterDecr)
{
	stringstream ss;
	ss << "DECRBY " << key << " " << decrement;
	return _exeCmdWithOutput(ss.str(), afterDecr);
}

int RedisCli::strGetBit(const string &key, int &bit)
{
	return _exeCmdWithOutput("GETBIT " + key, bit);
}

int RedisCli::strGetRange(const string &key, int start, int end, string &result)
{
	stringstream ss;
	ss << "GETRANGE " << key << " " << start << " " << end;
	return _exeCmdWithOutput(ss.str(), result);
}

int RedisCli::strIncrByFloat(const string &key, float increment, float &afterIncr)
{
	stringstream ss;
	ss << "INCRBYFLOAT " << key << " " << increment;
	return _exeCmdWithOutput(ss.str(), afterIncr);
}

int RedisCli::strMGet(const vector<string> &keys, vector<string> &values)
{
	stringstream ss;
	ss << "MGET ";
	vector<string>::const_iterator it;
	for (it = keys.begin(); it != keys.end(); it++) {
		ss << *it << " ";
	}
	return _exeCmdWithOutput(ss.str(), values);
}

int RedisCli::strMSet(const map<string, string> &kvs)
{
	stringstream ss;
	ss << "MSET ";
	map<string, string>::const_iterator it;
	for (it = kvs.begin(); it != kvs.end(); it++) {
		ss << it->first << " " << it->second << " ";
	}
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::strMSetNx(const map<string, string> &kvs)
{
	stringstream ss;
	ss << "MSETNX ";
	map<string, string>::const_iterator it;
	for (it = kvs.begin(); it != kvs.end(); it++) {
		ss << it->first << " " << it->second << " ";
	}
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::strPSetEx(const string &key, int64 milliSeconds, const string &value)
{
	stringstream ss;
	ss << "PSETEX " << key << " " << milliSeconds << " " << value;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::strSetBit(const string &key, int offset, const bool value)
{
	stringstream ss;
	ss << "SETBIT " << key << " " << offset << " " << value;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::strSetEx(const string &key, int seconds, const string &value)
{
	stringstream ss;
	ss << "SETEX " << key << " " << seconds << " " << value;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::strSetNx(const string &key, const string &value)
{
	return _exeCmdWithNoOutput("SETNX " + key + " " + value);
}

int RedisCli::strSetRange(const string &key, int offset, const string &value)
{
	stringstream ss;
	ss << "SETRANGE " << key << " " << offset << " " << value;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::strStrLen(const string &key)
{
	return _exeCmdWithNoOutput("STRLEN " + key);
}

int RedisCli::hashMdel(const string &key, const vector<string> &fields)
{
	stringstream ss;
	ss << "HDEL " << key << " ";
	vector<string>::const_iterator it;
	for (it = fields.begin(); it != fields.end(); it++) {
		ss << *it << " ";
	}
	return _exeCmdWithNoOutput(ss.str());
}

int64 RedisCli::hashIncrBy(const string &key, const string &field, 
		int64 increment, int64 &afterIncr)
{
	stringstream ss;
	ss << "HINCRBY " << key << " " << field << " " << increment;
	return _exeCmdWithOutput(ss.str(), afterIncr);
}

int RedisCli::hashIncrByFloat(const string &key, const string &field, 
		float increment, float &afterIncr)
{
	stringstream ss;
	ss << "HINCRBYFLOAT " << key << " " << field << " " << increment;
	return _exeCmdWithOutput(ss.str(), afterIncr);
}

int RedisCli::hashKeys(const string &key, vector<string> &fields)
{
	return _exeCmdWithOutput("HKEYS " + key, fields);
}

int RedisCli::hashLen(const string &key)
{
	return _exeCmdWithNoOutput("HLEN " + key);
}

int RedisCli::hashMGet(const string &key, const vector<string> &fields, vector<string> &values)
{
	stringstream ss;
	ss << "HMGET " << key << " ";
	vector<string>::const_iterator it;
	for (it = fields.begin(); it != fields.end(); it++) {
		ss << *it << " ";
	}
	return _exeCmdWithOutput(ss.str(), values);
}

int RedisCli::hashSetNx(const string &key, const string &field, const string &value)
{
	return _exeCmdWithNoOutput("HSETNX " + key + " " + field + " " + value);
}

int RedisCli::hashVals(const string &key, vector<string> &values)
{
	return _exeCmdWithOutput("HVALS " + key, values);
}

int RedisCli::listBLPop(const vector<string> &keys, int timeout, map<string, string> &mlv)
{
	stringstream ss;
	ss << "BLPOP ";
	vector<string>::const_iterator it;
	for (it = keys.begin(); it != keys.end(); it++) {
		ss << *it << " ";
	}
	ss << timeout;
	return _exeCmdWithOutput(ss.str(), mlv);
}

int RedisCli::listBRPop(const vector<string> &keys, int timeout, map<string, string> &mlv)
{
	stringstream ss;
	ss << "BRPOP ";
	vector<string>::const_iterator it;
	for (it = keys.begin(); it != keys.end(); it++) {
		ss << *it << " ";
	}
	ss << timeout;
	return _exeCmdWithOutput(ss.str(), mlv);
}

int RedisCli::listBRPopLPush(const string &srcList, const string &destList, 
  						int timeout, vector<string> &result)
{
	stringstream ss;
	ss << "BRPOPLPUSH " << srcList << " " << destList << " " << timeout;
	return _exeCmdWithOutput(ss.str(), result);
}

int RedisCli::listIndex(const string &key, int index, string &elmt)
{
	stringstream ss;
	ss << "LINDEX " << key << " " << index;
	return _exeCmdWithOutput(ss.str(), elmt);
}

int RedisCli::listInsert(const string &key, const string &position, const string &pivot, 
					const string &value)
{
	return _exeCmdWithNoOutput("LINSERT " + key + " " + position + " " + pivot + " " + value);
}

int RedisCli::listLen(const string &key)
{
	return _exeCmdWithNoOutput("LLEN " + key);
}

int RedisCli::listLPop(const string &key, string &head)
{
	return _exeCmdWithOutput("LPOP " + key, head);
}

int RedisCli::listLPush(const string &key, const vector<string> &elmts)
{
	stringstream ss;
	ss << "LPUSH " << key << " ";
	vector<string>::const_iterator it;
	for (it = elmts.begin(); it != elmts.end(); it++) {
		ss << *it << " ";
	}
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::listLPushx(const string &key, const string &elmt)
{
	return _exeCmdWithNoOutput("LPUSHX " + key + " " + elmt);
}

int RedisCli::listRange(const string &key, int start, int stop, vector<string> &elmts)
{
	stringstream ss;
	ss << "LRANGE " << key << " " << start << " " << stop;
	return _exeCmdWithOutput(ss.str(), elmts);
}

int RedisCli::listRem(const string &key, int count, const string &value)
{
	stringstream ss;
	ss << "LREM " << key << " " << count << " " << value;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::listRPop(const string &key, string &tail)
{
	return _exeCmdWithOutput("RPOP " + key, tail);
}

int RedisCli::listRPopLPush(const string &srcList, const string &destList, string &value)
{
	return _exeCmdWithOutput("RPOPLPUSH " + srcList + " " + destList, value);
}

int RedisCli::listRPush(const string &key, const vector<string> &elmts)
{
	stringstream ss;
	ss << "RPUSH " << key << " ";
	vector<string>::const_iterator it;
	for (it = elmts.begin(); it != elmts.end(); it++) {
		ss << *it << " ";
	}
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::listRPushx(const string &key, const string &elmt)
{
	return _exeCmdWithNoOutput("RPUSHX " + key + " " + elmt);
}

int RedisCli::listSet(const string &key, int index, const string &value)
{
	stringstream ss;
	ss << "LSET " << key << " " << index << " " << value;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::listTrim(const string &key, int start, int stop)
{
	stringstream ss;
	ss << "LTRIM " << key << " " << start << " " << stop;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::setCard(const string &key)
{
	return _exeCmdWithNoOutput("SCARD " + key);
}

int RedisCli::setDiff(const vector<string> &keys, vector<string> &members)
{
	stringstream ss;
	ss << "SDIFF " << " ";
	vector<string>::const_iterator it;
	for (it = keys.begin(); it != keys.end(); it++) {
		ss << *it << " ";
	}
	return _exeCmdWithOutput(ss.str(), members);
}

int RedisCli::setDiffStore(const string &destKey, const vector<string> &keys)
{
	stringstream ss;
	ss << "SDIFFSTORE " << destKey << " ";
	vector<string>::const_iterator it;
	for (it = keys.begin(); it != keys.end(); it++) {
		ss << *it << " ";
	}
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::setInter(const vector<string> &keys, vector<string> &members)
{
	stringstream ss;
	ss << "SINTER " << " ";
	vector<string>::const_iterator it;
	for (it = keys.begin(); it != keys.end(); it++) {
		ss << *it << " ";
	}
	return _exeCmdWithOutput(ss.str(), members);
}

int RedisCli::setInterStore(const string &destKey, const vector<string> &keys)
{
	stringstream ss;
	ss << "SINTERSTORE " << destKey << " ";
	vector<string>::const_iterator it;
	for (it = keys.begin(); it != keys.end(); it++) {
		ss << *it << " ";
	}
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::setMove(const string &srcKey, const string &destKey, const string &member)
{
	return _exeCmdWithNoOutput("SMOVE " + srcKey + " " + destKey + " " + member);
}

int RedisCli::setPop(const string &key, string &member)
{
	return _exeCmdWithOutput("SPOP " + key, member);
}

int RedisCli::setRandMember(const string &key, int count, vector<string> &members)
{
	stringstream ss;
	ss << "SRANDMEMBER " << " " << key << " " << count;
	return _exeCmdWithOutput(ss.str(), members);
}

int RedisCli::setUnion(const vector<string> &keys, vector<string> &members)
{
	stringstream ss;
	ss << "SUNION ";
	vector<string>::const_iterator it;
	for (it = keys.begin(); it != keys.end(); it++) {
		ss << *it << " ";
	}
	return _exeCmdWithOutput(ss.str(), members);
}

int RedisCli::setUnionStore(const string &destKey, const vector<string> &keys)
{
	stringstream ss;
	ss << "SUNIONSTORE " << destKey << " ";
	vector<string>::const_iterator it;
	for (it = keys.begin(); it != keys.end(); it++) {
		ss << *it << " ";
	}
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::ssetIncrBy(const string &key, double increment, 
		const string &member, string &afterIncr)
{
	stringstream ss;
	ss << "ZINCRBY " << key << " " << increment << " " << member;
	return _exeCmdWithOutput(ss.str(), afterIncr);
}

int RedisCli::ssetRangeByScore(const string &key, double minScore, double maxScore, 
  		const string &options, vector<string> &members)
{
	stringstream ss;
	ss << "ZRANGEBYSCORE " << key << " " << minScore << " " << maxScore << " " << options;
	return _exeCmdWithOutput(ss.str(), members);
}


int RedisCli::ssetRank(const string &key, const string &member)
{
	return _exeCmdWithNoOutput("ZRANK " + key + " " + member);
}

int RedisCli::ssetRem(const string &key, const vector<string> &members)
{
	stringstream ss;
	ss << "ZREM " << key << " ";
	vector<string>::const_iterator it;
	for (it = members.begin(); it != members.end(); it++) {
		ss << *it << " ";
	}
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::ssetRevRangeWithScore(const string &key, int start, int stop, vector<pair<string, string> > &members)
{
	stringstream ss;
	ss << "ZREVRANGE " << key << " " << start << " " << stop << " WITHSCORES";
	return _exeCmdWithOutput(ss.str(), members);
}

int RedisCli::ssetRevRangeByScore(const string &key, double maxScore, double minScore, 
  		const string &options, vector<string> &members)
{
	stringstream ss;
	ss << "ZREVRANGEBYSCORE " << key << " " << maxScore << " " << minScore << " " << options;
	return _exeCmdWithOutput(ss.str(), members);
}

int RedisCli::ssetRevRangeByScoreWithScore(const string &key, double maxScore, 
		double minScore, const string &options, vector<pair<string, string> > &members)
{
	stringstream ss;
	ss << "ZREVRANGEBYSCORE " << key << " " << maxScore << " " << minScore << " WITHSCORES " << options;
	return _exeCmdWithOutput(ss.str(), members);
}

int RedisCli::ssetRevRank(const string &key, const string &member)
{
	return _exeCmdWithNoOutput("ZREVRANK " + key + " " + member);
}

int RedisCli::ssetScore(const string &key, const string &member, string &score)
{
	return _exeCmdWithOutput("ZSCORE " + key + " " + member, score);
}

int RedisCli::connEcho(const string &msg)
{
	return _exeCmdWithNoOutput("ECHO " + msg);
}

int RedisCli::connSelect(int idx)
{
	stringstream ss;
	ss << "select " << idx;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::servBGRewriteAof()
{
	return _exeCmdWithNoOutput("BGREWRITEAOF");
}

int RedisCli::servCliGetName(string &name)
{
	return _exeCmdWithOutput("CLIENT GETNAME", name);
}

int RedisCli::servCliKill(const string &ip, int port)
{
	stringstream ss;
	ss << "CLIENT KILL " << ip << ":" << port;
	return _exeCmdWithNoOutput(ss.str());
}

int RedisCli::servCliList(vector<string> &cliInfos)
{
	return _exeCmdWithOutput("CLIENT LIST", cliInfos);
}

int RedisCli::servCliSetName(const string &name)
{
	return _exeCmdWithNoOutput("CLIENT SETNAME " + name);
}

int RedisCli::servCfgResetStat()
{
	return _exeCmdWithNoOutput("CONFIG RESETSTAT");
}

int RedisCli::servCfgRewrite()
{
	return _exeCmdWithNoOutput("CONFIG REWRITE");
}

int RedisCli::servFlushAll()
{
	return _exeCmdWithNoOutput("FLUSHALL");
}

int RedisCli::servFlushDB()
{
	return _exeCmdWithNoOutput("FLUSHDB");
}

int RedisCli::servLastSave(int &saveTime)
{
	return _exeCmdWithOutput("LASTSAVE", saveTime);
}

int RedisCli::servSave()
{
	return _exeCmdWithNoOutput("SAVE");
}

int RedisCli::servShutDown()
{
	return _exeCmdWithNoOutput("SHUTDOWN");
}

int RedisCli::servTime(vector<string> &ts)
{
	return _exeCmdWithOutput("TIME", ts);
}

#endif

RedisCli *connectCache(const string &ip, int port, const string &passWd)
{
	RedisCli *c = new RedisCli();
	if(c->connect(ip, port, passWd) < 0){
		delete c;
		return NULL;
	}
	return c;
}

};
