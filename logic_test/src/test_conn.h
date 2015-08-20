#ifndef __TEST_CONN_H__
#define __TEST_CONN_H__

#include "server_conn.h"

namespace gim{

class TestConn:public SvCon{
public:
	virtual int handleServiceRequest(const ServiceRequest& req);
	virtual int handleServiceResponse(const ServiceResponse& resp);
};


class TestConnFact:public SvConFactory{
public:
	virtual SvCon* createSvCon(void* par);
};

};

#endif/*__TEST_CONN_H__*/
