#include "ef_utility.h"
#include <sstream>
#ifdef	_WIN32
#include <time.h>
#include <windows.h>
#else
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#endif
namespace ef{
	int32 tv_cmp(struct timeval t1, struct timeval t2){
		if(t1.tv_sec < t2.tv_sec){
			return	-1;
		}else	if(t1.tv_sec > t2.tv_sec){
			return	1;
		}else{
			if(t1.tv_usec < t2.tv_usec){
				return	-1;
			}else	if(t1.tv_usec > t2.tv_usec){
				return	1;
			}else{
				return	0;
			}
		}
	}

	struct timeval tv_diff(struct timeval t1, struct timeval t2){
		struct	timeval	ret;

		ret.tv_sec = t1.tv_sec - t2.tv_sec;

		if(t1.tv_usec < t2.tv_usec){
			ret.tv_sec -= 1;
			t1.tv_usec += 1000000;
		}

		ret.tv_usec = t1.tv_usec - t2.tv_usec;

		return	ret;
	}

#ifdef _WIN32

	int gettimeofday(struct timeval *tp, struct timezone *tz)
	{
		time_t clock;
		struct tm tm;
		SYSTEMTIME wtm;

		GetLocalTime(&wtm);
		tm.tm_year     = wtm.wYear - 1900;
		tm.tm_mon     = wtm.wMonth - 1;
		tm.tm_mday     = wtm.wDay;
		tm.tm_hour     = wtm.wHour;
		tm.tm_min     = wtm.wMinute;
		tm.tm_sec     = wtm.wSecond;
		tm. tm_isdst    = -1;
		clock = mktime(&tm);
		tp->tv_sec = (long)clock;
		tp->tv_usec = wtm.wMilliseconds * 1000;

		return (0);
	}

#endif /*_WIN32*/

	int64 gettime_ms(){
		struct timeval tp;
		gettimeofday(&tp, NULL);

		return (int64)tp.tv_sec * 1000 + tp.tv_usec / 1000;
	}

	int split(const std::string& str, std::vector<std::string>& ret_, 
		std::string sep)
	{
	    if (str.empty())
	    {
		return 0;
	    }

	    std::string tmp;
	    std::string::size_type pos_begin = str.find_first_not_of(sep);
	    std::string::size_type comma_pos = 0;

	    while (pos_begin != std::string::npos)
	    {
		comma_pos = str.find(sep, pos_begin);
		if (comma_pos != std::string::npos)
		{
		    tmp = str.substr(pos_begin, comma_pos - pos_begin);
		    pos_begin = comma_pos + sep.length();
		}
		else
		{
		    tmp = str.substr(pos_begin);
		    pos_begin = comma_pos;
		}

		if (!tmp.empty())
		{
		    ret_.push_back(tmp);
		    tmp.clear();
		}
	    }
	    return 0;
	}
};

