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

RedisCG::RedisCG(const Json::Value &config, LogCb cb)
{
	init(config);
	setCmdLog(cb);
}

RedisCG::~RedisCG(){
	clear();
}

void RedisCG::setCmdLog(LogCb cb)
{
	m_cmdLog = cb;
}

int RedisCG::init(const Json::Value &config)
{
	Json::Reader reader;
        Json::Value value;

        if (!reader.parse(config.toStyledString(), value)) {
		return -1;
        }
	Json::Value urlArray = value["UrlList"];
	int cnt = 0;
	for (size_t i = 0; i < urlArray.size(); i++) {
		struct DBServ serv;
		if (parse_url(urlArray[i].asString(), serv) < 0) {
			continue;
		}
		m_cfg.servList.push_back(serv);
		cnt++;
	}
	m_dbs.clear();
	m_dbs.resize(cnt);
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
	ef::uint8 md5[16];
        ef::MD5_CTX c;

        ef::MD5Init(&c);
        ef::MD5Update(&c, (ef::uint8 *)key.data(), key.size());
        ef::MD5Final(md5, &c);
        //count the md5
        ef::uint64 i = *(ef::uint64*)md5;
        ef::uint64 idx = i % m_dbs.size();
	
	if(!m_dbs[idx]){
		m_dbs[idx] = connectCache(m_cfg.servList[idx].ipaddr, m_cfg.servList[idx].port, 
				m_cfg.servList[idx].passwd);
		if (!m_dbs[idx]) {
			if (m_cmdLog) {
				stringstream ss;
                                ss << "connect " << m_cfg.servList[idx].ipaddr << ":" << 
				m_cfg.servList[idx].port << ":" << m_cfg.servList[idx].passwd << " failed";
                                m_cmdLog(ss.str());
                        }
			return NULL;
		}
		if (m_cmdLog) {
			stringstream ss;
			ss << "connect " << m_cfg.servList[idx].ipaddr << ":" << 
				m_cfg.servList[idx].port << ":" << m_cfg.servList[idx].passwd << " success";
				m_cmdLog(ss.str());
			m_dbs[idx]->setCmdLog(m_cmdLog);
		}
	} 
	
	return  m_dbs[idx];
}

};
