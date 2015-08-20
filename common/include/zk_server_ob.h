#ifndef _ZK_SERVER_OBSERVER_H_
#define _ZK_SERVER_OBSERVER_H_
#include <json/json.h>
#include <map>
#include <vector>
#include "zk_watcher.h"

namespace gim
{
	class ZKClient;
	typedef std::map<std::string/*id*/, Json::Value>  children_map;

	typedef int (*ServerListObCallBack)(void* ctx, const children_map& children);

	class ZKServerListOb
		:public ZKWatcher
	{
	public:
		ZKServerListOb(ZKClient* c, const std::string& typepath);
		~ZKServerListOb();
		int init(bool loaddata, ServerListObCallBack cb, void* context);

		void getServerList(children_map& m);
		virtual int onChildrenChange(const std::vector<std::string>& children);
	private:
		void getServerList(const std::vector<std::string>& v, children_map& m);
	private:
		std::string m_path;

		ServerListObCallBack m_cb;
		void* m_ctx;
		bool m_loaddata;
	};
}
#endif // !_ZK_SERVER_OBSERVER_H_
