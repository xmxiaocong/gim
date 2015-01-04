#ifndef __DEF_USER_DB_H__
#define __DEF_USER_DB_H__

#include "user_db.h"

namespace gim{

class DefUserDB:public UserDB{
public:
	virtual ~DefUserDB(){}
	virtual int getUserInfo(const string &cid, string& key, 
		map<string, string>& pro){
		return 0;
	}
};

}

#endif/*__DEF_USER_DB_H__*/
