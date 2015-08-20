#ifndef __TOKENCHECKER_H__
#define __TOKENCHECKER_H__

#include <string>
#include <vector>
#include <map>
#include <time.h>
#include <pthread.h>
#include "base/ef_btype.h"

namespace gim {

#define AUTH_OK 0

//encode error
#define AUTH_ENCFAIL -1

//check error
#define AUTH_SIZE_ERROR -100
#define AUTH_CHECKSUM_ERROR -101
#define AUTH_TIMEOUT -102
#define AUTH_INVALID_INDEX -103
#define AUTH_DECFAIL -104

struct keyinfo {
	std::string str;
	time_t startTime;
	keyinfo():startTime(0){};
};

struct keylist{
	std::map<int, keyinfo> mkey;
	volatile time_t upTime;
	ef::uint16 idx;
	keylist():upTime(0),idx(0){};
};

class TokenChecker {
public:
	TokenChecker():m_timeout(7200),m_keyTimeout(600){
		pthread_mutex_init(&m_lock, NULL);
	};
	int setKey(ef::uint16 idx, const std::string &key);
	std::string getCurKey(void);
	int setTimeout(int timeout);
	int generateToken(const std::map<std::string, std::string> &minfo, std::string &token);
	int checkToken(const std::string &token, std::map<std::string, std::string> &minfo);
	keylist &getkeylist(){return m_keylist;};

protected:
	keylist m_keylist;
	int m_timeout;
	int m_keyTimeout;
	pthread_mutex_t m_lock;
	keylist *getMyKey();
};
		
const char *errPhase(int errcode);
};
#endif
