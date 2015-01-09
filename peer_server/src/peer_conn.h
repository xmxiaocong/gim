#ifndef __PEER_CONN_H__
#define __PEER_CONN_H__

#include "server_conn.h"

namespace gim{

class ServiceRequest;
class ServiceResponse;
class PeerPacket;

class PeerCon:public SvCon{
public:
	virtual int32 handleServiceRequest(const char* reqbody, 
		int32 len, std::string& respbody);
	virtual int32 handleServiceResponse(const char* respbody, 
		int32 len);
private:
	int32 handlePeerCmd(const ServiceRequest& svreq, 
		std::string& resppayload);

	int32 handleGetPeerMessage(const ServiceRequest& svreq,
		const PeerPacket& reqpk, PeerPacket& resppk);
	
	int32 handleSendPeerMessage(const ServiceRequest& svreq,
		const PeerPacket& reqpk, PeerPacket& resppk); 

	int32 handleRecvPeerMessage(const ServiceResponse& svresp,
		const PeerPacket& reqpk); 

	
};

class PeerConFac:public SvConFactory{
public:
	virtual PeerCon* createSvCon(void* par){
		return new PeerCon();
	}
};

SvConFactory* getPeerConFactory();

};

#endif/*PEER_CONN_H*/
