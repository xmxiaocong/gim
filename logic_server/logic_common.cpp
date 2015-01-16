#include "logic_common.h"
#include "msg_head.h"
#include "connect_server.pb.h"
#include "base/ef_utility.h" 
#include "base/ef_atomic.h"
#include "base/ef_base64.h"
#include "net/ef_sock.h"
#include <stdlib.h>
#include <sstream>

namespace gim{

	int32 constructPacket(const head& h, 
		const std::string& body, std::string& respbuf){
		head rh;
		respbuf.reserve(sizeof(h) + body.size());
		rh.cmd = htonl(h.cmd);
		rh.magic = htonl(h.magic); 		
		rh.len = htonl(sizeof(h) + body.size());
		respbuf.append((char*)&rh, sizeof(rh));
		respbuf.append(body);
		return 0;
	}


	int32 constructServiceRequest(const std::string& sessid,
		const std::string& tosessid,
		int32 service_type,
		const std::string& sn,
		const std::string& payload,
		std::string& req){
		ServiceRequest sreq;
		sreq.set_from_sessid(sessid);
		sreq.set_to_sessid(tosessid);
		sreq.set_svtype(service_type);
		sreq.set_sn(sn);
		sreq.set_payload(payload);
		if(!sreq.SerializeToString(&req))
			return -1;
		return 0;		
	}

	int32 constructServiceResponse(const std::string& sessid,
		const std::string& tosessid,
		int32 status,
		int32 service_type,
		const std::string& sn,
		const std::string& payload,
		std::string& req){
		ServiceResponse sreq;
		sreq.set_from_sessid(sessid);
		sreq.set_to_sessid(tosessid);
		sreq.set_status(status);
		sreq.set_svtype(service_type);
		sreq.set_sn(sn);
		sreq.set_payload(payload);
		if(!sreq.SerializeToString(&req))
			return -1;
		return 0;		
	}


};
