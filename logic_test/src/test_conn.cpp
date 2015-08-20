#include "test_conn.h"
#include "base/ef_utility.h"
#include "connect_server.pb.h"

namespace gim{

int TestConn::handleServiceRequest(const ServiceRequest& req){
	ServiceResponse resp;
	//call service
	ServiceRequest req1 = req;
	ServiceResponse resp1;
	req1.set_to_type(req.to_type() + 1);
	req1.set_from_type(req.to_type());
	req1.set_from_sessid(getSessID());
	int ret = callService(req1, resp1);		
		
	resp = resp1;
	resp.set_from_sessid(getSessID());
	resp.set_from_type(req.to_type());
	resp.set_status(ret);
	resp.set_to_sessid(req.from_sessid());

	ret = sendServiceResponse(resp);

	ALogError("TestConn") << "handleServiceRequest, <from_sessid:" 
		<< req.from_sessid() << "> <from_type:" << req.from_type() << "> <sn:"
		<< req.sn() << "> <payload:" << req.payload() << ">";
	//from client
	if(req.from_type() < 0){
		req1.set_from_type(req.to_type());
		req1.set_from_sessid(getSessID());
		req1.set_to_type(-1);
		req1.set_to_sessid(req.from_sessid());
		req1.set_sn(ef::itostr(ef::gettime_ms()));
		sendServiceRequest(req1);
	ALogError("TestConn") << "handleServiceRequest, <from_sessid:" 
		<< req.from_sessid() << "> <from_type:" << req.from_type() << "> <sn:"
		<< req.sn() << "> <payload:" << req.payload() << "> <req1_sn:" << req1.sn() 
		<< "> <req1_to_type:" << req.to_type() + 1 << "> <resp1_status:"
		<< resp1.status() << ">";
	}	

	return 0;

}


int TestConn::handleServiceResponse(const ServiceResponse& resp){
	ALogError("TestConn") << "handleServiceResponse, <from_sessid:" 
		<< resp.from_sessid() << "> <from_type:" << resp.from_type()
		<< "> <sn:" << resp.sn()<< "> <status:"<< resp.status() << ">"
		<< "> <payload:" << resp.payload() << ">";
	return 0;
}

SvCon* TestConnFact::createSvCon(void* par){
	return new TestConn();
}

};
