#include "ef_net_log.h"
#include "ef_common.h"

namespace ef{
	static std::string g_nlognm = "";


	void setNetLogName(const std::string& name){
		g_nlognm = name;
	}

	const std::string& getNetLogName(){
		return g_nlognm;
	}	

}

