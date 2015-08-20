#include "base/ef_aes.h"
#include "base/ef_base64.h"
#include "base/ef_utility.h"
#include "base/ef_tsd_ptr.h"
#include "time.h"
#include <sstream>
#include <fstream>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "token_checker.h"
#include "base/ef_atomic.h"
#include <iostream>

namespace gim{

using namespace std;
using namespace ef;

TSDPtr<keylist> g_keys;

static const uint64 seed = 0x0123456789abcdef;

const char *errPhase(int errcode)
{
	switch (errcode) {
		case AUTH_OK:
			return "OK";
		case AUTH_ENCFAIL:
			return "encrypt fail";
		case AUTH_SIZE_ERROR:
			return "invalid token head";
		case AUTH_CHECKSUM_ERROR:
			return "token checksum do not match";
		case AUTH_TIMEOUT:
			return "token timeout";
		case AUTH_INVALID_INDEX:
			return "invalid key index";
		case AUTH_DECFAIL:
			return "decrypt fail";
		default:
			return "unknown error";
	}
}

int TokenChecker::setKey(uint16 idx, const string &key)
{
	keylist *myKey = getMyKey();

	pthread_mutex_lock(&m_lock);
	
	if (m_keylist.mkey.find(idx) == m_keylist.mkey.end() || 
		(m_keylist.mkey[idx].str != key && myKey->idx != idx)) {
		m_keylist.mkey[idx].str = key;
		int32 tmdiff = time(NULL) - m_keylist.upTime;
		atomicAdd32((int32 *)&m_keylist.upTime, tmdiff);
	}
	pthread_mutex_unlock(&m_lock);
	return 0;
}
	

int TokenChecker::setTimeout(int timeout)
{
	m_timeout = timeout;
	return 0;
}

keylist *TokenChecker::getMyKey()
{
	keylist *myKey = g_keys.get();
	if (!myKey) {
		myKey = new keylist;
		pthread_mutex_lock(&m_lock);	
		myKey->mkey = m_keylist.mkey;
		myKey->upTime = m_keylist.upTime;
		pthread_mutex_unlock(&m_lock);
		g_keys.set(myKey);
	}
	
	if (myKey->upTime < m_keylist.upTime) {
		pthread_mutex_lock(&m_lock);
		myKey->mkey = m_keylist.mkey;
		myKey->upTime = m_keylist.upTime;
		pthread_mutex_unlock(&m_lock);
	}
	return myKey;
}

string TokenChecker::getCurKey(void)
{
	keylist *myKey = getMyKey();
	return myKey->mkey[myKey->idx].str;
}

int TokenChecker::generateToken(const map<string, string> &minfo, string &token)
{
	keylist *myKey = getMyKey();
	
	if (myKey->mkey[myKey->idx].startTime == 0) {
		myKey->mkey[myKey->idx].startTime = time(NULL);
	} else {
		if (time(NULL) - myKey->mkey[myKey->idx].startTime >= (m_timeout / myKey->mkey.size())) {
			myKey->idx = (myKey->idx + 1) % myKey->mkey.size();
			myKey->mkey[myKey->idx].startTime = time(NULL);
		}
	}
	
	string key = myKey->mkey[myKey->idx].str;
	stringstream ss;
	map<string, string>::const_iterator it = minfo.begin();
	for (; it != minfo.end(); it++)  {
		ss << it->first << "=" << it->second << "&";
	}
	string enctext;
	
	if (aesEncrypt(ss.str(), key, enctext) < 0) {
		return AUTH_ENCFAIL;
	}
	string str;
	time_t tm = time(NULL);
	str.append((char *)&tm, sizeof(tm));
	str.append((char *)&myKey->idx, sizeof(myKey->idx));
	uint64 checksum = tm ^ myKey->idx ^ seed;
	string padstr;
        padstr.append((char *)&enctext[0], enctext.size());
	if (enctext.size() % sizeof(checksum)) {
        	padstr.append(((enctext.size() / sizeof(checksum)) + 1) * sizeof(checksum) - enctext.size(), '\0');
	}
	for (int i = 0; i < padstr.size(); i+=sizeof(checksum)) {
		checksum ^= *((uint64 *)&padstr[i]);
	}
	str.append((char *)&checksum, sizeof(checksum));
	str.append((char *)&enctext[0], enctext.size());
	
	token = base64Encode(str);
	return AUTH_OK;  
}

int TokenChecker::checkToken(const string &token, map<string, string> &minfo)
{
	string decstr = base64Decode(token);
	if (decstr.size() <= sizeof(time_t) + sizeof(uint16) + sizeof(uint64)) {
		return AUTH_SIZE_ERROR;
	}
	
	time_t tm = *((time_t *)&decstr[0]);
	uint16 idx = *((uint16 *)&decstr[sizeof(tm)]);
	uint64 cks = *((uint64 *)&decstr[sizeof(tm) + sizeof(idx)]);
	

	cks ^= tm ^ idx ^ seed;
	string padstr;
	padstr.append((char *)&decstr[sizeof(tm) + sizeof(idx) + sizeof(cks)], 
		decstr.size() - sizeof(tm) - sizeof(idx) - sizeof(cks));
        if (padstr.size() % sizeof(cks)) {
                padstr.append(((padstr.size() / sizeof(cks)) + 1) * sizeof(cks) - padstr.size(), '\0');
        }
	for (int i = 0; i < padstr.size(); i+=sizeof(cks)) {
                cks ^= *((uint64 *)&padstr[i]);
        }
	if (cks) {
		return AUTH_CHECKSUM_ERROR;
	}
	
	if (time(NULL) - tm > m_timeout) {
		return AUTH_TIMEOUT;
	}
	
	keylist *myKey = getMyKey();

	if (idx >= myKey->mkey.size()) {
		return AUTH_INVALID_INDEX;
	}
	string rawbody;
	if (aesDecrypt(&decstr[sizeof(tm) + sizeof(idx) + sizeof(cks)], 
		decstr.size() - sizeof(tm) - sizeof(idx) - sizeof(cks), 
		myKey->mkey[idx].str, rawbody) < 0) {
		return AUTH_DECFAIL;
	}
	
	vector<string> vstr;
	split(rawbody, vstr, "&");
	vector<string>::iterator it = vstr.begin();
	for (; it != vstr.end(); it++) {
		string::size_type pos = it->find("=");
		if (pos != string::npos && pos < it->size() - 1) {
			minfo[it->substr(0, pos)] = it->substr(pos + 1);
		}
	}
	return AUTH_OK;
}

};
