#ifndef _PUSH_CONN_H_
#define _PUSH_CONN_H_
#include "server_conn.h"
#include <json/json.h>
#include "push_def.h"
#include "push_common.h"

namespace gim{
class PushSrvConn:public SvCon{
public:
	virtual int handleServiceRequest(const ServiceRequest& req);
	virtual int handleServiceResponse(const ServiceResponse& resp);
private:
	int handleGetMsgs(const ServiceRequest& req, const Json::Value& jargs);
	int sendJ2C(const Message& m);
	int handlePeer(const ServiceRequest& req, const Json::Value& jargs);
};

class PushSrvConnFactory:public SvConFactory{
public:
	virtual SvCon* createSvCon(void* par){
		return new PushSrvConn();
	}
};

}
#endif //end of _PUSH_CONN_H_