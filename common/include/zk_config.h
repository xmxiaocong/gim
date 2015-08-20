#ifndef _ZK_CFG_H_
#define _ZK_CFG_H_
#include <json/json.h>
#include <string>

namespace gim
{
	class ZKClient;
	class ZKDataWatcher;

	typedef void (*CfgChangedCallback)(void* ctx, int version, const Json::Value& notify);
	class ZKConfig
	{
	public:
		ZKConfig(ZKClient* c, const std::string& typepath, const std::string& srvid = "");
		~ZKConfig();
		int init(CfgChangedCallback cb, void* context);
		
		int getCfg(Json::Value& v);

	private:
		static int watchCallBack(void* ctx, int version, const std::string& data);
		int readNodeCfg(const std::string& path, Json::Value& v);
	private:
		ZKClient* m_cli;

		std::string m_typepath;

		std::string m_srvid;
		std::string m_srvpath;

		ZKDataWatcher* m_wt;
		ZKDataWatcher* m_ws;

		CfgChangedCallback m_cb;
		void* m_ctx;
	};
}

#endif // !_ZK_CFG_H_
