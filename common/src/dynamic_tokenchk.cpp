#include "dynamic_tokenchk.h"
#include "json/json.h"
#include <iostream>

namespace gim {

using namespace std;

void KeyWatcher::keyChangeCallBack(void *ctx, int version, const Json::Value &notify)
{
	kctx *c = (kctx *)ctx;
	c->tkchk->setKey(atoi(c->kwch->subpath().data()), notify["key"].asString());
}

KeyWatcher::KeyWatcher(ZKClient* c, const string& mainpath, const string &subpath, DynamicTokenChk *tkchk)
{
	m_mainpath = mainpath;
	m_subpath = subpath;
	m_zkcfg = new ZKConfig(c, m_mainpath + "/" + m_subpath);
	m_kctx = new kctx;
	m_kctx->tkchk = tkchk;
	m_kctx->kwch = this;
	m_zkcfg->init(keyChangeCallBack, (void *)m_kctx);
}

DynamicTokenChk::~DynamicTokenChk()
{
	if (m_zksrvob) delete m_zksrvob;
	if (m_zkcli) delete m_zkcli;
	map<string, KeyWatcher *>::iterator it = m_wchs.begin();
	for (; it != m_wchs.end(); it++) {
		if (it->second) delete it->second;
	}
}

int DynamicTokenChk::setWatcher(const string &path, KeyWatcher *kw)
{
	m_wchs[path] = kw;
	return 0;
}

int DynamicTokenChk::init(ZKClient *zkcli, const string &keyPath)
{
	m_keypath = keyPath;
	m_zkcli = zkcli;
	m_zksrvob = new ZKServerListOb(m_zkcli, m_keypath);
	m_zksrvob->init(true, childChangeCallBack, this);
	children_map mchild;
	m_zksrvob->getServerList(mchild);
	children_map::iterator it = mchild.begin();
	for (; it != mchild.end(); it++) {
		keyinfo tmpkinfo;
		tmpkinfo.str = it->second["key"].asString();
		m_keylist.mkey[atoi(it->first.data())] = tmpkinfo;
	}
	m_keylist.upTime = time(NULL);
		
	return 0;
}
	
int DynamicTokenChk::init(const string &zkurl, const string &keyPath)
{
	ZKClient *zkcli = new ZKClient();
	zkcli->init(zkurl);
	return init(zkcli, keyPath);
}

int DynamicTokenChk::childChangeCallBack(void *ctx, const children_map& children)
{
	DynamicTokenChk *c = (DynamicTokenChk *)ctx;
	children_map::const_iterator it = children.begin();
	for (; it != children.end(); it++) {
		c->setWatcher(it->first, new KeyWatcher(c->zkcli(), c->keypath(), it->first, c));
	}

	return 0;
}

/*int DynamicTokenChk::updateKey()
{
	return 0;
}*/

};
