#include "ef_net_log.h"
#include "ef_common.h"

namespace ef{
	static Appender* g_apd = NULL;	
	static int32 g_level = LOG_LEVEL_ALL;
	int32  initNetLog(int32 level, 
		const std::string& path, int32 span, bool fl){
		std::cout << "initNetLog, level:" << level << ", path:" << path
			<< std::endl;
		if(path.size()){
			g_apd = new FileAppender(path, span, fl);
		}else{
			g_apd = new ConsoleAppender();
		}
		g_level = level;
		return 0;
	}
	
	int32  initNetLog(const std::string& level, 
		const std::string& path, int32 span, bool fl){
		std::cout << "initNetLog, level:" << level << ", path:" << path
			<< std::endl;
		return initNetLog(getStrLevel(level.data()), path, span, fl);
	}

	Logger getNetLog(int level){
		Logger lg;
		if(level >= g_level){
			lg = Logger(NetFrameWorkLog, 
				LOG_TYPE_NORMAL, level, g_apd);
			lg.setHeader();
		}
		return lg;	
	}


}

