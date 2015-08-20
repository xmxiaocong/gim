#ifndef __DYNAMIC_TOKEN_CHECKER_H__
#define __DYNAMIC_TOKEN_CHECKER_H__

#include "token_checker.h"
#include "zk_client.h"
#include "zk_server_ob.h"
#include "zk_config.h"
#include <string>

namespace gim {

class DynamicTokenChk;
class KeyWatcher;;

struct kctx {
	DynamicTokenChk *tkchk;
	KeyWatcher *kwch;
	kctx():tkchk(NULL),kwch(NULL){};
};

class KeyWatcher {
public:
	KeyWatcher(ZKClient* c, const std::string &mainpath, const std::string &subpath,
		DynamicTokenChk *ctx);
	~KeyWatcher(){
		if (m_zkcfg) delete m_zkcfg;
		if (m_kctx) delete m_kctx;
	}
	std::string &subpath(){return m_subpath;};

private:
	ZKConfig *m_zkcfg;
	kctx *m_kctx;	
	std::string m_mainpath;
	std::string m_subpath;
	static void keyChangeCallBack(void* ctx, int version, const Json::Value& notify);
};

class DynamicTokenChk : public TokenChecker {
public:
	DynamicTokenChk():m_zkcli(NULL),m_zksrvob(NULL){};
	~DynamicTokenChk();
	int init(const std::string &zkUrl, const std::string &keyPath);
	int init(ZKClient *zkcli, const std::string &keyPath);
	std::string &keypath(){return m_keypath;};
	ZKClient *zkcli(){return m_zkcli;};
	int setWatcher(const std::string &path, KeyWatcher *kw);
private:
	static int childChangeCallBack(void *ctx, const children_map& children);
	
//	int updateKey();
	ZKClient *m_zkcli;
	ZKServerListOb *m_zksrvob;
	std::map<std::string, KeyWatcher *> m_wchs;
	std::string m_keypath;
};


};

#endif
