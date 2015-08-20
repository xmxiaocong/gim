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

int RedisCli::_exeCmd(Replyer &rpler, const char *fmt, va_list ap)
{
	int l = m_retry;
	int ret = 0;
	while(l-- > 0){
		if (!m_c) {
			ret = reconnect();
		}
		if (ret < 0) continue;
		ret = _doExeCmdNoReconnect(rpler, fmt, ap);
		if(ret >= 0 || ret == -4)
			return ret;
	}
	
	return ret;
}

int RedisCli::_exeCmd(Replyer &rpler, int argc, const char **argv, const size_t *argvLen)
{
	int l = m_retry;
	int ret = 0;
	while (l-- > 0) {
		if (!m_c) {
			ret = reconnect();
		}
		if (ret < 0) continue;
		ret = _doExeCmdNoReconnect(rpler, argc, argv, argvLen);
		if (ret >= 0 || ret == -4) {
			return ret;
		}
	}

	return ret;
}

int RedisCli::_doExeCmdNoReconnect(Replyer &rpler, const char *fmt, va_list ap)
{
	redisReply *tmpReply = NULL;
	va_list tmpap;

	if (ap == NULL) {
		tmpReply = (redisReply *)redisCommand(m_c, fmt);
	} else {
		va_copy(tmpap, ap);
		tmpReply = (redisReply *)redisvCommand(m_c, fmt, ap);	
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
	return 0;

}

int RedisCli::_doExeCmdNoReconnect(Replyer &rpler, int argc, const char **argv, 
		const size_t *argvLen)
{
        redisReply *tmpReply = (redisReply *)redisCommandArgv(m_c, argc, argv, argvLen);
		
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
        return 0;
}

static size_t _argSize(const vector<string> &value)
{
        return value.size();
}

static size_t _argSize(const map<string, string> &value)
{
        return value.size() * 2;
}

static void _fillData(int &curPos, const char **argv, size_t  *argvLen,
                vector<string>::const_iterator it)
{
        argv[curPos] = it->data();
        argvLen[curPos] = it->size();
}

static void _fillData(int &curPos, const char **argv, size_t  *argvLen,
                map<string, string>::const_iterator it)
{
        argv[curPos] = it->first.data();
        argvLen[curPos] = it->first.size();
        curPos++;
        argv[curPos] = it->second.data();
        argvLen[curPos] = it->second.size();
}

int RedisCli::_exeCmdWithNoOutput(const char *fmt, ...)
{
	int ret;
	Replyer rpler;
	va_list ap;

	va_start(ap, fmt);
	ret = _exeCmd(rpler, fmt, ap);
	va_end(ap);
	if (ret < 0) {
		return -1;
	}
	return 0;
}

int RedisCli::_exeCmdWithNoOutput(const string &cmd)
{
	int ret;
	Replyer rpler;
	
	ret = _exeCmd(rpler, cmd.data(), NULL);
	if (ret < 0) {
		return -1;
	}
	return 0;
}

template <typename T>
int RedisCli::_exeCmdWithNoOutputM(const string &cmdName, const string &key, const T&input)
{
        int ret = 0;
        int argc = 0;
        const char **argv = NULL;
        size_t *argvLen = NULL;
        int i = 0;
        Replyer rpler;
	typename T::const_iterator it = input.begin();

        argc = _argSize(input) + 1;
        if (!key.empty()) argc++;

        argv = (const char **)malloc(sizeof(char *) * argc);
        if (!argv) {
		ret = -1;
                goto exit;
        }

        argvLen = (size_t *)malloc(sizeof(size_t) * argc);
        if (!argvLen) {
		ret = -1;
                goto exit;
        }

        argv[i] = cmdName.data();
        argvLen[i++] = cmdName.size();
        if (!key.empty()) {
                argv[i] = key.data();
                argvLen[i++] = key.size();
        }
	
        for (; it != input.end(); it++, i++) {
                _fillData(i, argv, argvLen, it);
        }

        ret = _exeCmd(rpler, argc, argv, argvLen);
	if (ret < 0) {
		ret = -1;
	} else {
		ret = 0;
	}
exit:
        if (argv) free(argv);
        if (argvLen) free(argvLen);
        return ret;
}

template <typename T>
int RedisCli::_exeCmdWithOutput(T &output, const char *fmt, ...)
{
	int ret = 0;
	Replyer rpler;
	va_list ap;

	va_start(ap, fmt);
	ret = _exeCmd(rpler, fmt, ap);
	va_end(ap);
	if (ret < 0) {
		return -1;
	}
	return getResultFromReply(rpler.reply(), output);
}

template <typename T, typename U>
int RedisCli::_exeCmdWithOutputM(T &output, const string &cmdName, const string &key, 
		const U&input)
{
        int ret = 0;
        int argc = 0;
        const char **argv = NULL;
        size_t *argvLen = NULL;
        int i = 0;
        Replyer rpler;
	typename U::const_iterator it = input.begin();

        argc = _argSize(input) + 1;
        if (!key.empty()) argc++;

        argv = (const char **)malloc(sizeof(char *) * argc);
        if (!argv) {
		ret = -1;
                goto exit;
        }

        argvLen = (size_t *)malloc(sizeof(size_t) * argc);
        if (!argvLen) {
		ret = -1;
                goto exit;
        }

        argv[i] = cmdName.data();
        argvLen[i++] = cmdName.size();
        if (!key.empty()) {
                argv[i] = key.data();
                argvLen[i++] = key.size();
        }
	
        for (; it != input.end(); it++, i++) {
                _fillData(i, argv, argvLen, it);
        }

        ret = _exeCmd(rpler, argc, argv, argvLen);
        if (ret < 0) {
                ret = -1;
        } else {
       		ret = getResultFromReply(rpler.reply(), output);
	}
exit:
        if (argv) free(argv);
        if (argvLen) free(argvLen);
        return ret;
}

template <typename T>
int RedisCli::_exeCmdWithOutput(T &output, const string &cmd)
{
        int ret = 0;
        Replyer rpler;

        ret = _exeCmd(rpler, cmd.data(), NULL);
        if (ret < 0) {
                return -1;
        }
        return getResultFromReply(rpler.reply(), output);
}

int RedisCli::keyDel(const string &key)
{
	return _exeCmdWithNoOutput("DEL %b", key.data(), key.size());
}

int RedisCli::keyExists(const string &key, int &exists)
{
	return _exeCmdWithOutput(exists, "EXISTS %b", key.data(), key.size());
}

int RedisCli::keyExpire(const string &key, int seconds)
{
	return _exeCmdWithNoOutput("EXPIRE %b %d", key.data(), key.size(), seconds);
}

int RedisCli::strGet(const string &key, string &value)
{
	return _exeCmdWithOutput(value, "GET %b", key.data(), key.size());
}

int RedisCli::strGetSet(const string &key, const string &value, string &oldvalue)
{
	return _exeCmdWithOutput(oldvalue, "GETSET %b %b", key.data(), key.size(), 
		value.data(), value.size());
}

int RedisCli::strIncr(const string &key, int64 &afterIncr)
{
	return _exeCmdWithOutput(afterIncr, "INCR %b", key.data(), key.size());
}

int RedisCli::strIncrBy(const string &key, int64 increment, int64 &afterIncr)
{
	return _exeCmdWithOutput(afterIncr, "INCRBY %b %lld", key.data(), key.size(), increment);
}

int RedisCli::strSet(const string &key, const string &value)
{
	return _exeCmdWithNoOutput("SET %b %b", key.data(), key.size(), 
		value.data(), value.size());
}

int RedisCli::hashDel(const string &key, const string &field)
{
	return _exeCmdWithNoOutput("HDEL %b %b", key.data(), key.size(), 
		field.data(), field.size());
}

int RedisCli::hashMDel(const string &key, const vector<string> &fields)
{
	return _exeCmdWithNoOutputM("HDEL", key, fields);
}

int RedisCli::hashExists(const string &key, const string &field, int &exists)
{
	return _exeCmdWithOutput(exists, "HEXISTS %b %b", key.data(), key.size(), 
		field.data(), field.size());
}

int RedisCli::hashGet(const string &key, const string &field, string &value)
{
	return _exeCmdWithOutput(value, "HGET %b %b", key.data(), key.size(),
		field.data(), field.size());
}

int RedisCli::hashGetAll(const string &key, map<string, string> &mfv)
{
	return _exeCmdWithOutput(mfv, "HGETALL %b", key.data(), key.size());
}

int RedisCli::hashMSet(const string &key, const map<string, string> &mfv)
{
	return _exeCmdWithNoOutputM("HMSET", key, mfv);
}

int RedisCli::hashSet(const string &key, const string &field, const string &value)
{
	return _exeCmdWithNoOutput("HSET %b %b %b", key.data(), key.size(),
		field.data(), field.size(), value.data(), value.size());
}

/* set related */
int RedisCli::setAdd(const string &key, const vector<string> &members)
{
	return _exeCmdWithNoOutputM("SADD", key, members);
}

int RedisCli::setIsMember(const string &key, const string &member, int &ismember)
{
	return _exeCmdWithOutput(ismember, "SISMEMBER %b %b", key.data(), key.size(),
		 member.data(), member.size());
}	

int RedisCli::setMembers(const string &key, vector<string> &members)
{
	return _exeCmdWithOutput(members, "SMEMBERS %b", key.data(), key.size());
}

int RedisCli::setRem(const string &key, const vector<string> &members)
{
	return _exeCmdWithNoOutputM("SREM", key, members);
}

/* sorted set related */
int RedisCli::ssetAdd(const string &key, const map<string, string> &members)
{
	return _exeCmdWithNoOutputM("ZADD", key, members);
}

int RedisCli::ssetCard(const string &key, int64 &card)
{
	return _exeCmdWithOutput(card, "ZCARD %b", key.data(), key.size());
}

int RedisCli::ssetCount(const string &key, double minScore, double maxScore, int64 &count)
{
	if (minScore == DBL_MIN) {
		if (maxScore == DBL_MAX) {
			return _exeCmdWithOutput(count, "ZCOUNT %b -inf +inf", key.data(), key.size());
		} else {
			return _exeCmdWithOutput(count, "ZCOUNT %b -inf %f", key.data(), key.size(), maxScore);
		}
	} else {
		if (maxScore == DBL_MAX) {
			return _exeCmdWithOutput(count, "ZCOUNT %b %f +inf", key.data(), key.size(), minScore);
		} else {
			return _exeCmdWithOutput(count, "ZCOUNT %b %f %f", key.data(), key.size(), minScore, maxScore);
		}	
	}
}

int RedisCli::ssetRange(const string &key, int start, int stop, vector<string> &members)
{
	return _exeCmdWithOutput(members, "ZRANGE %b %d %d", key.data(), key.size(), start, stop);
}

int RedisCli::ssetRangeByScoreLimit(const string &key, double minScore, double maxScore,
                int offset, int length, vector<string> &members)
{
         if (minScore == DBL_MIN) {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithOutput(members, "ZRANGEBYSCORE %b -inf +inf LIMIT %d %d",
                                key.data(), key.size(), offset, length);
                } else {
                        return _exeCmdWithOutput(members, "ZRANGEBYSCORE %b -inf %f LIMIT %d %d",
                                key.data(), key.size(), maxScore, offset, length);
                }
        } else {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithOutput(members, "ZRANGEBYSCORE %b %f +inf LIMIT %d %d",
                                key.data(), key.size(), minScore, offset, length);
                } else {
                        return _exeCmdWithOutput(members, "ZRANGEBYSCORE %b %f %f LIMIT %d %d",
                                key.data(), key.size(), minScore, maxScore, offset, length);
                }
        }
}

int RedisCli::ssetRangeWithScore(const string &key, int start, int stop, vector<pair<string, string> > &members)
{
	return _exeCmdWithOutput(members, "ZRANGE %b %d %d WITHSCORES", key.data(), key.size(), start, stop);
}

int RedisCli::ssetRangeByScoreWithScore(const string &key, double minScore, double maxScore, 
		vector<pair<string, string> > &members)
{
	if (minScore == DBL_MIN) {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithOutput(members, "ZRANGEBYSCORE %b -inf +inf WITHSCORES", 
				key.data(), key.size());
                } else {
                        return _exeCmdWithOutput(members, "ZRANGEBYSCORE %b -inf %f WITHSCORES", 
				key.data(), key.size(), maxScore);
                }
        } else {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithOutput(members, "ZRANGEBYSCORE %b %f +inf WITHSCORES", 
				key.data(), key.size(), minScore);
                } else {
                        return _exeCmdWithOutput(members, "ZRANGEBYSCORE %b %f %f WITHSCORES", 
				key.data(), key.size(), minScore, maxScore);
                }
        }
}

int RedisCli::ssetRangeByScoreWithScoreLimit(const string &key, double minScore, double maxScore, 
		int offset, int length, vector<pair<string, string> > &members)
{
	if (minScore == DBL_MIN) {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithOutput(members, "ZRANGEBYSCORE %b -inf +inf WITHSCORES LIMIT %d %d", 
				key.data(), key.size(), offset, length);
                } else {
                        return _exeCmdWithOutput(members, "ZRANGEBYSCORE %b -inf %f WITHSCORES LIMIT %d %d", 
				key.data(), key.size(), maxScore, offset, length);
                }
        } else {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithOutput(members, "ZRANGEBYSCORE %b %f +inf WITHSCORES LIMIT %d %d", 
				key.data(), key.size(), minScore, offset, length);
                } else {
                        return _exeCmdWithOutput(members, "ZRANGEBYSCORE %b %f %f WITHSCORES LIMIT %d %d", 
				key.data(), key.size(), minScore, maxScore, offset, length);
                }
        }
}

int RedisCli::ssetRemRange(const string &key, int start, int stop)
{
	return _exeCmdWithNoOutput("ZREMRANGEBYRANK %b %d %d", key.data(), key.size(), start, stop);
}

int RedisCli::ssetRemRangeByScore(const string &key, double minScore, double maxScore)
{
	if (minScore == DBL_MIN) {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithNoOutput("ZREMRANGEBYSCORE %b -inf +inf", key.data(), key.size());
                } else {
                        return _exeCmdWithNoOutput("ZREMRANGEBYSCORE %b -inf %f", key.data(), key.size(), maxScore);
                }
        } else {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithNoOutput("ZREMRANGEBYSCORE %b %f +inf", key.data(), key.size(), minScore);
                } else {
                        return _exeCmdWithNoOutput("ZREMRANGEBYSCORE %b %f %f", key.data(), key.size(), minScore, maxScore);
                }
        }
}

int RedisCli::ssetRevRange(const string &key, int start, int stop, vector<string> &members)
{
	return _exeCmdWithOutput(members, "ZREVRANGE %b %d %d", key.data(), key.size(), start, stop);
}

int RedisCli::ssetRevRangeByScoreLimit(const string &key, double maxScore, double minScore,
                int offset, int length, vector<string> &members)
{
         if (minScore == DBL_MIN) {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithOutput(members, "ZREVRANGEBYSCORE %b +inf -inf LIMIT %d %d",
                                key.data(), key.size(), offset, length);
                } else {
                        return _exeCmdWithOutput(members, "ZREVRANGEBYSCORE %b %f -inf LIMIT %d %d",
                                key.data(), key.size(), maxScore, offset, length);
                }
        } else {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithOutput(members, "ZREVRANGEBYSCORE %b +inf %f LIMIT %d %d",
                                key.data(), key.size(), minScore, offset, length);
                } else {
                        return _exeCmdWithOutput(members, "ZREVRANGEBYSCORE %b %f %f LIMIT %d %d",
                                key.data(), key.size(), maxScore, minScore, offset, length);
                }
        }
}

int RedisCli::ssetRevRangeByScoreWithScore(const string &key, double maxScore, 
		double minScore, vector<pair<string, string> > &members)
{
	 if (minScore == DBL_MIN) {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithOutput(members, "ZREVRANGEBYSCORE %b +inf -inf WITHSCORES", 
				key.data(), key.size());
                } else {
                        return _exeCmdWithOutput(members, "ZREVRANGEBYSCORE %b %f -inf WITHSCORES", 
				key.data(), key.size(), maxScore);
                }
        } else {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithOutput(members, "ZREVRANGEBYSCORE %b +inf %f WITHSCORES", 
				key.data(), key.size(), minScore);
                } else {
                        return _exeCmdWithOutput(members, "ZREVRANGEBYSCORE %b %f %f WITHSCORES", 
				key.data(), key.size(), maxScore, minScore);
                }
        }
}

int RedisCli::ssetRevRangeByScoreWithScoreLimit(const string &key, double maxScore, 
		double minScore, int offset, int length, vector<pair<string, string> > &members)
{
	 if (minScore == DBL_MIN) {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithOutput(members, "ZREVRANGEBYSCORE %b +inf -inf WITHSCORES LIMIT %d %d",
                                key.data(), key.size(), offset, length);
                } else {
                        return _exeCmdWithOutput(members, "ZREVRANGEBYSCORE %b %f -inf WITHSCORES LIMIT %d %d",
                                key.data(), key.size(), maxScore, offset, length);
                }
        } else {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithOutput(members, "ZREVRANGEBYSCORE %b +inf %f WITHSCORES LIMIT %d %d",
                                key.data(), key.size(), minScore, offset, length);
                } else {
                        return _exeCmdWithOutput(members, "ZREVRANGEBYSCORE %b %f %f WITHSCORES LIMIT %d %d",
                                key.data(), key.size(), maxScore, minScore, offset, length);
                }
        }
}

/* connection related */
int RedisCli::connAuth(const string &passWord)
{
	Replyer rpler;
	string str("AUTH " + passWord);
	return _doExeCmdNoReconnect(rpler, str.data(), NULL);
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
	return _exeCmdWithOutput(results, "CONFIG GET %s", parameter.data());
}

int RedisCli::servCfgSet(const string &parameter, const string &value)
{
	return _exeCmdWithNoOutput("CONFIG SET %s %b", parameter.data(), value.data(), value.size());
}

int RedisCli::servDBSize()
{
	return _exeCmdWithNoOutput("DBSIZE");
}

int RedisCli::servSlaveOf(const string &masterip, int masterport)
{
	return _exeCmdWithNoOutput("SLAVEOF %s %d", masterip.data(), masterport);
}

int RedisCli::servSlaveOfNoOne()
{
	return _exeCmdWithNoOutput("SLAVEOF NO ONE");
}

int RedisCli::transDiscard()
{
	return _exeCmdWithNoOutput("DISCARD");
}

int RedisCli::transExec()
{
	return _exeCmdWithNoOutput("EXEC");
}

int RedisCli::transMulti()
{
	return _exeCmdWithNoOutput("MULTI");
}

int RedisCli::transWatch(const string &key)
{
	return _exeCmdWithNoOutput("WATCH %b", key.data(), key.size());
}

int RedisCli::transUnwatch()
{
	return _exeCmdWithNoOutput("UNWATCH");
}

#ifdef ENABLE_UNSTABLE_INTERFACE

int RedisCli::keyDump(const string &key, string &value)
{
	return _exeCmdWithOutput(value, "DUMP %b", key.data(), key.size());
}

int RedisCli::keyExpireAt(const string &key, int64 timeStamp)
{
	return _exeCmdWithNoOutput("EXPIREAT %b %lld", key.data(), key.size(), timeStamp);
}

int RedisCli::keyKeys(const string &pattern, vector<string> &keys)
{
	return _exeCmdWithOutput(keys, "KEYS %s", pattern.data());
}

int RedisCli::keyMigrate(const string &host, int port, const string &key, 
  		int destDB, int timeout)
{
	return _exeCmdWithNoOutput("MIGRATE %s %d %b %d %d", host.data(), port,
			key.data(), key.size(), destDB, timeout);
}

int RedisCli::keyMove(const string &key, int destDB)
{
	return _exeCmdWithNoOutput("MOVE %b %d", key.data(), key.size(), destDB);
}

int RedisCli::keyObject(const string &subCmd, const string &key, vector<string> &result)
{
	return _exeCmdWithOutput(result, "OBJECT %s %b", subCmd.data(), key.data(), key.size());
}

int RedisCli::keyPersist(const string &key)
{
	return _exeCmdWithNoOutput("PERSIST %b", key.data(), key.size());
}

int RedisCli::keyPExpire(const string &key, int64 milliSeconds)
{
	return _exeCmdWithNoOutput("PEXPIRE %b %lld", key.data(), key.size(), milliSeconds);
}

int RedisCli::keyPExpireAt(const string &key, int64 milliTimeStamp)
{
	return _exeCmdWithNoOutput("PEXPIREAT %b %lld", key.data(), key.size(), milliTimeStamp);
}

int RedisCli::keyPTTL(const string &key, int64 &milliTTL)
{
	return _exeCmdWithOutput(milliTTL, "PTTL %b", key.data(), key.size());
}

int RedisCli::keyRandomKey(string &key)
{
	return _exeCmdWithOutput(key, "RANDOMKEY");
}

int RedisCli::keyRename(const string &key, const string &newKey)
{
	return _exeCmdWithNoOutput("RENAME %b %b", key.data(), key.size(), newKey.data(), newKey.size());
}

int RedisCli::keyRenameNx(const string &key, const string &newKey)
{
	return _exeCmdWithNoOutput("RENAMENX %b %b", key.data(), key.size(), newKey.data(), newKey.size());
}

int RedisCli::keyRestore(const string &key, int ttl, const string &value)
{
	return _exeCmdWithNoOutput("RESTORE %b %d %b", key.data(), key.size(), ttl, value.data(), value.size());
}

int RedisCli::keyTTL(const string &key, int &ttl)
{
	return _exeCmdWithOutput(ttl, "TTL %b", key.data(), key.size());
}

int RedisCli::keyType(const string &key, string &type)
{
	return _exeCmdWithOutput(type, "TYPE %b", key.data(), key.size());
}

int RedisCli::strAppend(const string &key, const string &value)
{
	return _exeCmdWithNoOutput("APPEND %b %b", key.data(), key.size(), value.data(), value.size());
}

int RedisCli::strBitCount(const string &key, int start, int end)
{
	return _exeCmdWithNoOutput("BITCOUNT %b %d %d", key.data(), key.size(), start, end);
}

int RedisCli::strBitOp(const string &op, const string &destKey, const vector<string> &keys)
{
	vector<string> varg = keys;
	varg.insert(varg.begin(), destKey);
	return _exeCmdWithNoOutputM("BITOP", op, varg);
}

int RedisCli::strDecr(const string &key, int64 &afterDecr)
{
	return _exeCmdWithOutput(afterDecr, "DECR %b", key.data(), key.size());
}

int RedisCli::strDecrBy(const string &key, int64 decrement, int64 &afterDecr)
{
	return _exeCmdWithOutput(afterDecr, "DECRBY %b %lld", key.data(), key.size(), decrement);
}

int RedisCli::strGetBit(const string &key, int &bit)
{
	return _exeCmdWithOutput(bit, "GETBIT %b", key.data(), key.size());
}

int RedisCli::strGetRange(const string &key, int start, int end, string &result)
{
	return _exeCmdWithOutput(result, "GETRANGE %b %d %d", key.data(), key.size(), start, end);
}

int RedisCli::strIncrByFloat(const string &key, float increment, float &afterIncr)
{
	return _exeCmdWithOutput(afterIncr, "INCRBYFLOAT %b %f", key.data(), key.size(), increment);
}

int RedisCli::strMGet(const vector<string> &keys, vector<string> &values)
{
	return _exeCmdWithOutputM(values, "GET", "", keys);
}

int RedisCli::strMSet(const map<string, string> &kvs)
{
	return _exeCmdWithNoOutputM("SET", "", kvs);
}

int RedisCli::strMSetNx(const map<string, string> &kvs)
{
	return _exeCmdWithNoOutputM("SETNX", "", kvs);
}

int RedisCli::strPSetEx(const string &key, int64 milliSeconds, const string &value)
{
	return _exeCmdWithNoOutput("PSETEX %b %lld %b", key.data(), key.size(),
		milliSeconds, value.data(), value.size());
}

int RedisCli::strSetBit(const string &key, int offset, const bool value)
{
	return _exeCmdWithNoOutput("SETBIT %b %d %d", key.data(), key.size(), offset, value);
}

int RedisCli::strSetEx(const string &key, int seconds, const string &value)
{
	return _exeCmdWithNoOutput("SETEX %b %d %b", key.data(), key.size(), seconds, 
		value.data(), value.size());
}

int RedisCli::strSetNx(const string &key, const string &value)
{
	return _exeCmdWithNoOutput("SETNX %b %b", key.data(), key.size(), value.data(), value.size());
}

int RedisCli::strSetRange(const string &key, int offset, const string &value)
{
	return _exeCmdWithNoOutput("SETRANGE %b %d %b", key.data(), key.size(),
		offset, value.data(), value.size());
}

int RedisCli::strLen(const string &key, int &len)
{
	return _exeCmdWithOutput(len, "STRLEN " + key);
}

int RedisCli::hashIncrBy(const string &key, const string &field, 
		int64 increment, int64 &afterIncr)
{
	return _exeCmdWithOutput(afterIncr, "HINCRBY %b %b %lld", key.data(), key.size(),
		field.data(), field.size(), increment);
}

int RedisCli::hashIncrByFloat(const string &key, const string &field, 
		float increment, float &afterIncr)
{
	return _exeCmdWithOutput(afterIncr, "HINCRBYFLOAT %b %b %f", key.data(), key.size(),
		field.data(), field.size(), increment);
}

int RedisCli::hashKeys(const string &key, vector<string> &fields)
{
	return _exeCmdWithOutput(fields, "HKEYS %b", key.data(), key.size());
}

int RedisCli::hashLen(const string &key, int &len)
{
	return _exeCmdWithOutput(len, "HLEN %b", key.data(), key.size());
}

int RedisCli::hashMGet(const string &key, const vector<string> &fields, vector<string> &values)
{
	return _exeCmdWithOutputM(values, "HGET", key, fields);
}

int RedisCli::hashSetNx(const string &key, const string &field, const string &value)
{
	return _exeCmdWithNoOutput("HSETNX %b %b %b", key.data(), key.size(),
		field.data(), field.size(), value.data(), value.size());
}

int RedisCli::hashVals(const string &key, vector<string> &values)
{
	return _exeCmdWithOutput(values, "HVALS %b", key.data(), key.size());
}

int RedisCli::listBLPop(const vector<string> &keys, int timeout, map<string, string> &mlv)
{
	vector<string> varg;
	string str;
	str.append((char *)&timeout, sizeof(timeout));
	varg = keys;
	varg.push_back(str);
	return _exeCmdWithOutputM(mlv, "BLPOP", "", varg);
}

int RedisCli::listBRPop(const vector<string> &keys, int timeout, map<string, string> &mlv)
{
	vector<string> varg;
	string str;
	str.append((char *)&timeout, sizeof(timeout));
	varg = keys;
	varg.push_back(str);
	return _exeCmdWithOutputM(mlv, "BRPOP", "", varg);
}

int RedisCli::listBRPopLPush(const string &srcList, const string &destList, 
  						int timeout, vector<string> &result)
{
	return _exeCmdWithOutput(result, "BRPOPLPUSH %b %b %d", srcList.data(), srcList.size(),
		destList.data(), destList.size(), timeout);
}

int RedisCli::listIndex(const string &key, int index, string &elmt)
{
	return _exeCmdWithOutput(elmt, "LINDEX %b %d", key.data(), key.size(), index);
}

int RedisCli::listInsert(const string &key, const string &position, const string &pivot, 
					const string &value)
{
	return _exeCmdWithNoOutput("LINSERT %b %b %b %b", key.data(), key.size(),
		position.data(), position.size(), pivot.data(), pivot.size(), value.data(), value.size());
}

int RedisCli::listLen(const string &key)
{
	return _exeCmdWithNoOutput("LLEN %b", key.data(), key.size());
}

int RedisCli::listLPop(const string &key, string &head)
{
	return _exeCmdWithOutput(head, "LPOP %b", key.data(), key.size());
}

int RedisCli::listLPush(const string &key, const vector<string> &elmts)
{
	return _exeCmdWithNoOutputM("LPUSH", key, elmts);
}

int RedisCli::listLPushx(const string &key, const string &elmt)
{
	return _exeCmdWithNoOutput("LPUSHX %b %b", key.data(), key.size(),
		elmt.data(), elmt.size());
}

int RedisCli::listRange(const string &key, int start, int stop, vector<string> &elmts)
{
	return _exeCmdWithOutput(elmts, "LRANGE %b %d %d", key.data(), key.size(), start, stop);
}

int RedisCli::listRem(const string &key, int count, const string &value)
{
	return _exeCmdWithNoOutput("LREM %b %d %b", key.data(), key.size(), count, value.data(), value.size());
}

int RedisCli::listRPop(const string &key, string &tail)
{
	return _exeCmdWithOutput(tail, "RPOP %b", key.data(), key.size());
}

int RedisCli::listRPopLPush(const string &srcList, const string &destList, string &value)
{
	return _exeCmdWithOutput(value, "RPOPLPUSH " + srcList + " " + destList);
}

int RedisCli::listRPush(const string &key, const vector<string> &elmts)
{
	return _exeCmdWithNoOutputM("RPUSH", key, elmts);
}

int RedisCli::listRPushx(const string &key, const string &elmt)
{
	return _exeCmdWithNoOutput("RPUSHX %b %b", key.data(), key.size(),
		elmt.data(), elmt.size());
}

int RedisCli::listSet(const string &key, int index, const string &value)
{
	return _exeCmdWithNoOutput("LSET %b %d %b", key.data(), key.size(), index,
		value.data(), value.size());
}

int RedisCli::listTrim(const string &key, int start, int stop)
{
	return _exeCmdWithNoOutput("LTRIM %b %d %d", key.data(), key.size(),
		start, stop);
}

int RedisCli::setCard(const string &key, int64 &card)
{
	return _exeCmdWithOutput(card, "SCARD %b", key.data(), key.size());
}

int RedisCli::setDiff(const vector<string> &keys, vector<string> &members)
{
	return _exeCmdWithOutputM(members, "SDIFF", "", keys);
}

int RedisCli::setDiffStore(const string &destKey, const vector<string> &keys)
{
	return _exeCmdWithNoOutputM("SDIFFSTORE", destKey, keys);
}

int RedisCli::setInter(const vector<string> &keys, vector<string> &members)
{
	return _exeCmdWithOutputM(members, "SINTER", "", keys);
}

int RedisCli::setInterStore(const string &destKey, const vector<string> &keys)
{
	return _exeCmdWithNoOutputM("SINTERSTORE", destKey, keys);
}

int RedisCli::setMove(const string &srcKey, const string &destKey, const string &member)
{
	return _exeCmdWithNoOutput("SMOVE %b %b %b", srcKey.data(), srcKey.size(),
		destKey.data(), destKey.size(), member.data(), member.size());
}

int RedisCli::setPop(const string &key, string &member)
{
	return _exeCmdWithOutput(member, "SPOP %b", key.data(), key.size());
}

int RedisCli::setRandMember(const string &key, int count, vector<string> &members)
{
	return _exeCmdWithOutput(members, "SRANDMEMBER %b %d", key.data(), key.size(), count);
}

int RedisCli::setUnion(const vector<string> &keys, vector<string> &members)
{
	return _exeCmdWithOutputM(members, "SUNION", "", keys);
}

int RedisCli::setUnionStore(const string &destKey, const vector<string> &keys)
{
	return _exeCmdWithNoOutputM("SUNIONSTORE", destKey, keys);
}

int RedisCli::ssetIncrBy(const string &key, double increment, 
		const string &member, string &afterIncr)
{
	return _exeCmdWithOutput(afterIncr, "ZINCRBY %b %f %b", key.data(), key.size(),
		increment, member.data(), member.size());
}

int RedisCli::ssetRangeByScore(const string &key, double minScore, double maxScore, 
  		vector<string> &members)
{
	 if (minScore == DBL_MIN) {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithOutput(members, "ZRANGEBYSCORE %b -inf +inf", key.data(), key.size());
                } else {
                        return _exeCmdWithOutput(members, "ZRANGEBYSCORE %b -inf %f", key.data(), key.size(), maxScore);
                }
        } else {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithOutput(members, "ZRANGEBYSCORE %b %f +inf", key.data(), key.size(), minScore);
                } else {
                        return _exeCmdWithOutput(members, "ZRANGEBYSCORE %b %f %f", key.data(), key.size(), minScore, maxScore);
                }
        }
}

int RedisCli::ssetRank(const string &key, const string &member, int64 &rank)
{
	return _exeCmdWithOutput(rank, "ZRANK %b %b", key.data(), key.size(), 
		member.data(), member.size());
}

int RedisCli::ssetRem(const string &key, const vector<string> &members)
{
	return _exeCmdWithNoOutputM("ZREM", key, members);
}

int RedisCli::ssetRevRangeWithScore(const string &key, int start, int stop, 
	vector<pair<string, string> > &members)
{
	return _exeCmdWithOutput(members, "ZREVRANGE %b %d %d WITHSCORES", 
		key.data(), key.size(), start, stop);
}

int RedisCli::ssetRevRangeByScore(const string &key, double maxScore, double minScore, 
  		vector<string> &members)
{
	 if (minScore == DBL_MIN) {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithOutput(members, "ZREVRANGEBYSCORE %b +inf -inf", key.data(), key.size());
                } else {
                        return _exeCmdWithOutput(members, "ZREVRANGEBYSCORE %b %f -inf", key.data(), key.size(), maxScore);
                }
        } else {
                if (maxScore == DBL_MAX) {
                        return _exeCmdWithOutput(members, "ZREVRANGEBYSCORE %b +inf %f", key.data(), key.size(), minScore);
                } else {
                        return _exeCmdWithOutput(members, "ZREVRANGEBYSCORE %b %f %f", key.data(), key.size(), maxScore, minScore);
                }
        }
}

int RedisCli::ssetRevRank(const string &key, const string &member, int64 &rank)
{
	return _exeCmdWithOutput(rank, "ZREVRANK %b %b", key.data(), key.size(), 
		member.data(), member.size());
}

int RedisCli::ssetScore(const string &key, const string &member, string &score)
{
	return _exeCmdWithOutput(score, "ZSCORE %b %b", key.data(), key.size(),
		member.data(), member.size());
}

int RedisCli::connEcho(const string &msg)
{
	return _exeCmdWithNoOutput("ECHO %b", msg.data(), msg.size());
}

int RedisCli::connSelect(int idx)
{
	return _exeCmdWithNoOutput("SELECT %d", idx);
}

int RedisCli::servBGRewriteAof()
{
	return _exeCmdWithNoOutput("BGREWRITEAOF");
}

int RedisCli::servCliGetName(string &name)
{
	return _exeCmdWithOutput(name, "CLIENT GETNAME");
}

int RedisCli::servCliKill(const string &ip, int port)
{
	return _exeCmdWithNoOutput("CLIENT KILL %s %d", ip.data(), port);
}

int RedisCli::servCliList(vector<string> &cliInfos)
{
	return _exeCmdWithOutput(cliInfos, "CLIENT LIST");
}

int RedisCli::servCliSetName(const string &name)
{
	return _exeCmdWithNoOutput("CLIENT SETNAME %b", name.data(), name.size());
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
	return _exeCmdWithOutput(saveTime, "LASTSAVE");
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
	return _exeCmdWithOutput(ts, "TIME");
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
