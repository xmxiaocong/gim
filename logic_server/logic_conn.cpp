#include "logic_conn.h"
#include "logic_server.h"
#include "dispatcher.h"
#include "err_no.h"

namespace gim{

Dispatcher* LogicCon::getDispatcher(){
	return (Dispatcher*)getEventLoop()->getObj();
}

int LogicCon::sendServiceRequestToClient(const std::string& cid,
	int svtype, const std::string& sn,
	const std::string& payload,
	const std::string& callback){
	Dispatcher* d = getDispatcher();	

	if(!d){
		return INNER_ERROR;
	}

	int ret = d->sendServiceRequestToClient(cid, svtype,
		sn, payload, callback);

	return ret;

}

int LogicCon::sendServiceResponseToClient(const std::string& cid,
	int svtype, int status, const std::string& sn,
	const std::string& payload,
	const std::string& callback){
	Dispatcher* d = getDispatcher(); 

	if(!d){
		return INNER_ERROR;
	}

	int ret = d->sendServiceResponseToClient(cid, svtype,
		sn, status, payload, callback);

	return ret;

}

};
