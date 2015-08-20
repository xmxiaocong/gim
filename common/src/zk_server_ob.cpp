#include "zk_server_ob.h"
#include "zk_client.h"

namespace gim
{
	ZKServerListOb::ZKServerListOb(ZKClient* c, const std::string& typepath)
		:ZKWatcher(typepath), m_path(typepath),
		m_cb(NULL), m_ctx(NULL), m_loaddata(false)
	{
		addToClient(c);
	}
	ZKServerListOb::~ZKServerListOb()
	{
	}
	int ZKServerListOb::init(bool loaddata, ServerListObCallBack cb, void* context)
	{
		m_loaddata = loaddata;
		m_cb = cb;
		m_ctx = context;
		addWatch(ZKWatcher::CHILD_EVENT);
		return 0;
	}
	void ZKServerListOb::getServerList(children_map& m){
		s_vector v;
		getClient()->getChildren(m_path, v);
		getServerList(v, m);
	}
	void ZKServerListOb::getServerList(const std::vector<std::string>& v, children_map& m){
		for (unsigned int n = 0; n<v.size(); n++)
		{
			if (m_loaddata)
			{
				std::string p = zkPath(m_path, v[n]);
				std::string data;
				getClient()->getNodeData(p, data);
				try{
					Json::Value jv;
					Json::Reader r;
					if (r.parse(data, jv)){
						m[v[n]] = jv;
						continue;
					}
				}
				catch (const std::exception &ex){
					if(m_cli)
						m_cli->writeLog(std::string("ZKServerListOb::getServerList exception:") + ex.what());
				}
			}
			m[v[n]] = Json::Value();
		}
	}
	int ZKServerListOb::onChildrenChange(const std::vector<std::string>& children){
		children_map m;
		getServerList(children, m);
		if(m_cb)
			return m_cb(m_ctx, m);
		return 0;
	}

}//end of ef