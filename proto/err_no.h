#ifndef __ERR_NO_H__
#define __ERR_NO_H__


namespace lr_com{


enum{
	STATUS_OK = 0,
	INPUT_FORMAT_ERROR = -1,
	CREATE_SESSION_FAIL = -10,
	INVLID_SESSION_ID = -20,
	SESSION_TIMEOUT = -21, 
	NO_SERVICE = -30,
	THIS_SERVICE_EMPTY = -31,
	SERVICE_EVENTLOOP_NULL = -32,
	SERVICE_TOO_BUSY = -33,
	INVALID_SN = -40,
	SN_TIMEOUT = -41,
	INNER_ERROR = -100,
};

inline const char* getErrStr(int e){
	switch(e){
	case STATUS_OK:
		return "STATUS_OK";
	case INPUT_FORMAT_ERROR:
		return "INPUT_FORMAT_ERROR";
	case CREATE_SESSION_FAIL:
		return "CREATE_SESSION_FAIL";
	case INVLID_SESSION_ID:
		return "INVLID_SESSION_ID";
	case NO_SERVICE:
		return "NO_SERVICE";
	case THIS_SERVICE_EMPTY:
		return "THIS_SERVICE_EMPTY";
	case SERVICE_EVENTLOOP_NULL:
		return "SERVICE_EVENTLOOP_NULL";
	case SESSION_TIMEOUT:
		return "SESSION_TIMEOUT";
	case SERVICE_TOO_BUSY:
		return "SERVICE_TOO_BUSY";
	case INVALID_SN:
		return "INVALID_SN";
	case SN_TIMEOUT:
		return "SN_TIMEOUT";
	case INNER_ERROR:
		return "INNER_ERROR";
	}
	return "";
}

};


#endif
