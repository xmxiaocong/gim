#include "zk_server_node.h"
#include "zk_client.h"

namespace gim
{
	ZKServerNode::ZKServerNode(ZKClient* c, const std::string& typepath, const std::string& id)
		:m_cli(c)
	{
		m_path = zkPath(typepath, id);
	}

	ZKServerNode::~ZKServerNode()
	{
		if (m_cli)
		{
			m_cli->deleteNode(m_path);
		}
	}
	int ZKServerNode::init(const std::string& data)
	{
		if (m_cli)
			return m_cli->createEphemeralNode(m_path, data);

		return -1;
	}

	int ZKServerNode::setData(const std::string& data)
	{
		int ret = -1;
		if (m_cli){
			ret = m_cli->setNodeData(m_path, data);
			if(ZNONODE == ret){
				ret = m_cli->createEphemeralNode(m_path, data);
			}
		}
		return ret;
	}
	int ZKServerNode::setJson(const Json::Value& v) 
	{
		try
		{
			Json::FastWriter w; 
			return setData(w.write(v));
		}
		catch (const std::exception &ex)
		{
			if(m_cli)
				m_cli->writeLog(std::string("ZKServerNode::setJson() exception:") + ex.what());
			return -1;
		}
	}
}
