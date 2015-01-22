#ifndef _CLIENT_SDK_LOG_H_
#define _CLIENT_SDK_LOG_H_

#include <stdarg.h>
#include "client_def.h"
#include <string>
#include <stdio.h>

namespace gim
{
	typedef enum _LogLevel
	{
		LOG_LEVEL_TRACE = 1,
		LOG_LEVEL_WARN,
		LOG_LEVEL_DEBUG,
		LOG_LEVEL_ERROR
	}LogLevel;

	void logprint(LogLevel level, const char* logbuf);

#ifdef _DEBUG
#define SDK_LOG(lvl, format, ...)\
			{\
				char buf[1024];\
				snprintf(buf, sizeof(buf), "[lvl:%d] " format"        [src=%s:%d]\n", (int32)lvl, ##__VA_ARGS__, __FILE__, __LINE__ );\
				logprint(lvl, buf); \
			}
#else
#define SDK_LOG(lvl, format, ...)\
			{\
				if(lvl >= LOG_LEVEL_ERROR)\
				{\
					char buf[1024];\
					snprintf(buf, sizeof(buf), "[lvl:%d] " format"     [src=%s:%d]\n", (int32)lvl, ##__VA_ARGS__, __FILE__, __LINE__ );\
					logprint(lvl, buf); \
				}\
			}
#endif

}


#endif

