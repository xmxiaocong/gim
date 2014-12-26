#include "redis_cg.h"
#include "base/ef_md5.h"
#include "base/ef_btype.h"
#include "base/ef_utility.h"
#include <stdlib.h>
#include <sstream>

namespace gim {

using namespace ef;

static int parse_url(const string& url, DBServ &serv)
{
	vector<string> s_v;
	split(url, s_v, ":");

	if (s_v.size() < 2) return -1;
	serv.ipaddr = s_v[0];
	serv.port = atoi(s_v[1].data());
	if (s_v.size() > 2) {
		serv.passwd = s_v[2];
	}
	
	return 0;
}

RedisCG::~RedisCG(){
	clear();
}

int RedisCG::init(const vector<string> &servList)
{
	if (servList.size() <= 0) return -1;

	vector<string>::const_iterator it;
	for (it = servList.begin(); it != servList.end(); it++) {
		struct DBServ serv;
		if (parse_url(*it, serv) < 0) {
			continue;
		}
		m_servList.push_back(serv);
	}
	m_dbs.clear();
	m_dbs.resize(m_servList.size());
	for(size_t i = 0; i < m_dbs.size(); ++i){
		m_dbs[i] = NULL;
	}
	
	return 0;	
}

int RedisCG::clear()
{
	for(size_t i = 0; i < m_dbs.size(); ++i){
		if(m_dbs[i])
			delete  m_dbs[i];
	}

	return 0;
}

DBHandle RedisCG::getHndl(const string &key)
{
	uint64 idx = getIdx(m_dbs.size(), key);
	
	if(!m_dbs[idx]){
		m_dbs[idx] = connectCache(m_servList[idx].ipaddr, m_servList[idx].port, 
				m_servList[idx].passwd);
		if (!m_dbs[idx]) {
			if (m_cmdLog) {
				stringstream ss;
                                ss << "connect " << m_servList[idx].ipaddr << ":" << 
				m_servList[idx].port << ":" << m_servList[idx].passwd << " failed";
                                m_cmdLog(ss.str());
                        }
			return NULL;
		}
		if (m_cmdLog) {
			stringstream ss;
			ss << "connect " << m_servList[idx].ipaddr << ":" << 
				m_servList[idx].port << ":" << m_servList[idx].passwd << " success";
				m_cmdLog(ss.str());
			m_dbs[idx]->setCmdLog(m_cmdLog);
		}
	} 
	
	return  m_dbs[idx];
}

};
