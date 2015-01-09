#ifndef __PEER_CMD_H__
#define __PEER_CMD_H__

namespace gim{

enum{
	SERVICE_TYPE_PEER = 200,
};

enum{
	SEND_PEER_MESSAGE_REQ = 110,
	SEND_PEER_MESSAGE_RESP = 111,
	GET_PEER_MESSAGE_REQ = 112,
	GET_PEER_MESSAGE_RESP = 113,
};

enum{
	MSG_LEFT_COUNT = 30,
};


#define DEFAULT_MSG_COUNT (5)

#define PeerErrorLog(reqsn, cmd, cid, status) ALogError("PeerServer")\
		<< "<sn:" << reqsn\
		<< "> <action:" << cmd << "> <cid:"\
		<< cid << "> <status:"\
		<< (int)status << "> <errstr:"\
		<< getErrStr(status) << "> <FILE:"\
		<< __FILE__ << "> <LINE:" << __LINE__\
		<< ">"

#define FormatErrorLog(reqsn, cmd)	ALogError("PeerServer")\
			<< "<sn:" << reqsn << "> <action:"\
			<< cmd << "> <status:"\
			<< (int)INPUT_FORMAT_ERROR\
			<< "> <errstr:INPUT_FORMAT_ERROR> <FILE:"\
			<< __FILE__ << "> <LINE:" << __LINE__\
			<< ">"



};

#endif/*PEER_CMD_H*/
