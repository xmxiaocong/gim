#include "logic_common.h"
#include "msg_head.h"
#include "proto/connect_server.pb.h"
#include "common/ef_utility.h" 
#include "common/ef_base64.h"
#include "common/ef_sock.h"
#include <stdlib.h>
#include <sstream>

namespace gim
{
	int32 constructRespPacket(int32 srvcmd, const std::string& body, std::string& respbuf)
	{
		head rh;
		respbuf.reserve(sizeof(head) + body.size());
		rh.cmd = htonl(srvcmd);
		rh.magic = htonl(MAGIC_NUMBER);
		rh.len = htonl(sizeof(rh) + body.size());
		respbuf.append((char*)&rh, sizeof(rh));
		respbuf.append(body);
		return 0;
	}

	int32 constructReqPacket(const head& h, const std::string& body, std::string& respbuf)
	{
		head rh;
		respbuf.reserve(sizeof(h) + body.size());
		rh.cmd = htonl(h.cmd);
		rh.magic = htonl(h.magic); 		
		rh.len = htonl(sizeof(h) + body.size());
		respbuf.append((char*)&rh, sizeof(rh));
		respbuf.append(body);
		return 0;
	}


	int32 constructServiceRequest(const std::string& sessid, int32 service_type, 
		const std::string& sn, const std::string& payload, std::string& req)
	{
		ServiceRequest sreq;
		sreq.set_sessid(sessid);
		sreq.set_svtype(service_type);
		sreq.set_sn(sn);
		sreq.set_payload(payload);
		if(!sreq.SerializeToString(&req))
			return -1;
		return 0;		
	}

	int32 constructServiceResponse(const std::string& sessid,int32 status,int32 service_type,
		const std::string& sn,const std::string& payload,std::string& req)
	{
		ServiceResponse sreq;
		sreq.set_sessid(sessid);
		sreq.set_status(status);
		sreq.set_svtype(service_type);
		sreq.set_sn(sn);
		sreq.set_payload(payload);
		if(!sreq.SerializeToString(&req))
			return -1;
		return 0;		
	}
};
