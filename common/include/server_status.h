#ifndef __SERVER_STATUS_H__
#define __SERVER_STATUS_H__

#include <vector>
#include <map>
#include <string>
#include <json/json.h>

namespace Json{
    class Value;
};


namespace gim{
    struct PortConfig{
        int Port;
        int MaxType;
        int MinType;
        std::map<std::string, std::string> others;
    };

    struct ServerStatus{
        int ID;
        std::vector<std::string> IPs;
        std::vector<PortConfig> Ports;
        std::map<std::string, std::string> Properties;

        int parseFromJson(const Json::Value& v);
        int serializeToJson(Json::Value& v);
    };

}

#endif/*__SERVER_STATUS_H__*/
