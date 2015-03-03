#include "user_db.h"
#include "def_user_db.h"

namespace gim{

UserDBFactory* UserDBFactory::g_fct = NULL;

int UserDBFactory::init(const Json::Value& conf){
	g_fct = create(conf);
	return 0;
}

void UserDBFactory::free(){
	delete g_fct;
	g_fct = NULL;
}

UserDBFactory* UserDBFactory::get(){
	return g_fct;
}

UserDBFactory* UserDBFactory::create(const Json::Value& config){
	const Json::Value& type = config["Type"];
	
	if(!type.isString()){
		return NULL;
	}
	
	const Json::Value& conf = config["Config"];

	if(type.asString() == "DefaultType"){
		DefUserDBFactory* c = new DefUserDBFactory(config);
		return c;
		
	}

	return NULL;
}

}
