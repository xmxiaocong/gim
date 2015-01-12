#ifndef	EF_UTILITY_H
#define EF_UTILITY_H

#include "ef_btype.h"

#ifndef	_WIN32
#include <sys/time.h>
#include <time.h>
#endif
#include <sstream>
#include <string>
#include <vector>

namespace ef{

#ifdef _WIN32
	int gettimeofday(struct timeval *tp, struct timezone *tz);
#endif

	int64 gettime_ms();

	int tv_cmp(struct timeval t1, struct timeval t2);

	struct timeval tv_diff(struct timeval t1, struct timeval t2);

	int split(const std::string& str, std::vector<std::string>& ret_, 
		std::string sep = ",");

	template<class T>
	std::string itostr(const T& t)
	{
		std::stringstream oss;
		oss << t;
		return oss.str();
	}
};

#endif/*EF_UTILITY_H*/

