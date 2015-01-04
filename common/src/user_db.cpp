#include "user_db.h"
#include "def_user_db.h"

namespace gim{

UserDB* UserDBFactory::getUserDB(const Json::Value& config){
	const Json::Value& type = config["Type"];
	
	if(!type.isString()){
		return NULL;
	}
	
	const Json::Value& conf = config["Config"];

	if(type.asString() == "DefaultType"){
		DefUserDB* c = new DefUserDB();
		return c;
	}

	return NULL;
}

}
