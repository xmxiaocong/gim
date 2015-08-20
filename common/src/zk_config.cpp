#include "zk_config.h"
#include "zk_client.h"
#include "zk_watcher.h"

namespace gim
{
	ZKConfig::ZKConfig(ZKClient* c, const std::string& typepath, const std::string& srvid)
		:m_cli(c), m_typepath(typepath), m_srvid(srvid),
		m_wt(NULL), m_ws(NULL),
		m_cb(NULL), m_ctx(NULL)
	{
	}
	ZKConfig::~ZKConfig()
	{
		if (m_wt)
			delete m_wt;
		if (m_ws)
			delete m_ws;
	}
	int ZKConfig::init(CfgChangedCallback cb, void* context)
	{
		m_cb = cb;
		m_ctx = context;

		m_wt = new ZKDataWatcher(m_typepath);
		m_wt->addToClient(m_cli);
		m_wt->initNodeData();

		if(!m_srvid.empty()){
			m_srvpath = zkPath(m_typepath,m_srvid);
			m_ws = new ZKDataWatcher(m_srvpath);
			m_ws->addToClient(m_cli);
			m_ws->initNodeData();
		}

		m_wt->addWatch(ZKWatcher::CHANGE_EVENT);
		m_wt->setCb(this, ZKConfig::watchCallBack);
		if(m_ws){
			m_ws->addWatch(ZKWatcher::CHANGE_EVENT);
			m_ws->setCb(this, ZKConfig::watchCallBack);
		}

		return 0;
	}

	int ZKConfig::getCfg(Json::Value& v)
	{
		readNodeCfg(m_wt->getData(), v);

		if(m_ws)
			readNodeCfg(m_ws->getData(), v);

		return 0;
	}
	
	int ZKConfig::readNodeCfg(const std::string& data, Json::Value& v){
		try{
			Json::Value vtmp;
			Json::Reader r;
			if (r.parse(data, vtmp) && vtmp.type() == Json::objectValue){
				Json::Value::Members m = vtmp.getMemberNames();
				for (unsigned int n = 0; n < m.size(); ++n)
				{
					v[m[n]] = vtmp[m[n]];
				}
				return 0;
			}
		}
		catch (const std::exception &ex){
			if(m_cli)
				m_cli->writeLog(std::string("ZKConfig::readNodeCfg exception:") + ex.what());
		}
		return -1;
	}
	int ZKConfig::watchCallBack(void* ctx, int version, const std::string& data){
		ZKConfig* __this = (ZKConfig*) ctx;
		Json::Value v;
		__this->getCfg(v);
		__this->m_cb(__this->m_ctx, version, v);
		return 0;
	}
} //end of ef
