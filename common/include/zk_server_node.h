#ifndef _ZK_SERVER_NODE_H_
#define _ZK_SERVER_NODE_H_
#include <string>
#include <json/json.h>

namespace gim
{
	class ZKClient;

	class ZKServerNode
	{
	public:
		ZKServerNode(ZKClient* c, const std::string& typepath, const std::string& id);
		~ZKServerNode();
		int init(const std::string& data);
		int setData(const std::string& data);
		int setJson(const Json::Value& v);
	private:
		ZKClient* m_cli;
		std::string m_path;
	};
}

#endif
