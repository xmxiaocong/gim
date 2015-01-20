#ifndef __PEER_CONN_H__
#define __PEER_CONN_H__

#include "server_conn.h"

namespace gim{

class ServiceRequest;
class ServiceResponse;
class PeerPacket;

class PeerCon:public SvCon{
public:
	virtual int handleServiceRequest(const std::string& payload);
	virtual int handleServiceResponse(int svtype, int status,
		 const std::string& payload, const std::string&);
private:
	int handleGetPeerMessage(const PeerPacket& reqpk);
	
	int handleSendPeerMessage(const PeerPacket& reqpk); 

	int handleRecvPeerMessage(const PeerPacket& reqpk); 

	
};

int getPeerMsg(const std::string& cid, int64 startid, 
	int64 cnt, vector<Message>& msg, int64& last_msg_id);
int sendPeerMsg(const std::string& sn, const Message& cid, 
	int64& msgid);


class PeerConFac:public SvConFactory{
public:
	virtual PeerCon* createSvCon(void* par){
		return new PeerCon();
	}
};

SvConFactory* getPeerConFactory();

};

#endif/*PEER_CONN_H*/
