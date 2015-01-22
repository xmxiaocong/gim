#ifndef SERVICE_TYPE_H
#define SERVICE_TYPE_H

namespace gim
{
	typedef enum _ServiceType
	{
		SERVICE_PUSH = 100,
		SERVICE_PEER = 200,
		SERVICE_GROUP = 300,
		SERVICE_VERIFY = 500,
		SERVICE_TOPIC	= 600
	}ServiceType;
};

#endif
