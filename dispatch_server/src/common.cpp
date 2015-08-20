#include "common.h"
#include "net/ef_connection.h"
#include "proto/msg_head.h"
#include "base/ef_utility.h" 
#include "base/ef_base64.h"
#include <stdlib.h>
#include <sstream>

namespace gim{

	int32 checkLen(ef::Connection& c){
		head h;
		if(c.bufLen() < (int32)sizeof(h)){
			return 0;
		}
		c.peekBuf((char*)&h, sizeof(h));
		h.magic = htonl(h.magic);
		h.len = htonl(h.len);
		if(h.len < (int32)sizeof(h) 
			|| h.len > 1024 * 1024){
			return -1;
		}
		if(h.len <= c.bufLen()){
			return h.len;
		}
		return 0;
	}


	int32 constructPacket(const head& h, 
		const std::string& body, std::string& respbuf){
		head rh;
		respbuf.reserve(sizeof(h) + body.size());
		rh.cmd = htonl(h.cmd);
		rh.magic = h.magic; 		
		rh.len = htonl(sizeof(h) + body.size());
		respbuf.append((char*)&rh, sizeof(rh));
		respbuf.append(body);
		return 0;
	}

	int32 decorationName(int32 svid, int32 conid,
		const std::string& n, std::string& dn){
		
		std::string buf;
		buf.resize(8);

		char* p = (char*)buf.data();
		
		*(int32*)p = svid;
		*(int32*)(p + sizeof(int32)) = conid;

		dn = base64Encode(buf) + n;
		return 0;
	}

	int32 getDecorationInfo(const std::string& dn, int32& svid,
		int32& conid, std::string& n){

		std::string b64buf = dn.substr(0, 12);

		std::string buf = base64Decode(b64buf); 

		char* p = (char*)buf.data();
		svid = *(int32*)p;
		conid = *(int32*)(p + sizeof(int32));

		n = dn.substr(12);		

		return 0;
	}	


};
