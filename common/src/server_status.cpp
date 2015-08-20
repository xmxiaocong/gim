#include "server_status.h"
#include <json/json.h>
#include <sstream>
#include <ctype.h>
#include <stdlib.h>
#include <iostream>

namespace gim{

using namespace std;

static int is_digit(const string &instr)
{
    if (instr.size() == 0) return 0;

    if (!isdigit(instr[0]) && instr[0] != '-') {
        return 0;
    }

    for (int i = 1; i < instr.size(); i++) {
        if (!isdigit(instr[i])) return 0;
    }

    return 1;
}

int ServerStatus::parseFromJson(const  Json::Value & v){
    Json::Value::Members mblist = v.getMemberNames();
    Json::Value::Members::iterator it = mblist.begin();

    for (; it != mblist.end(); it++) {
        const Json::Value &jv = v[*it];
        if (*it == "ID") {
            ID = jv.asInt();
        } else if (*it == "IPs") {
            for (int i = 0; i < jv.size(); i++) {
                IPs.push_back(jv[i].asString());
            }
        } else if (*it == "CliConfigs") {
            for (int i = 0; i < jv.size(); i++) {
                struct PortConfig pc;
                const Json::Value &servcfg = jv[i];
                const Json::Value::Members cl = servcfg.getMemberNames();
                Json::Value::Members::const_iterator nit = cl.begin();

                for (; nit != cl.end(); nit++) {
                    if (*nit == "ListenPort") {
                        pc.Port = servcfg[*nit].asInt();
                    } else if (*nit == "MaxType") {
                        pc.MaxType = servcfg[*nit].asInt();
                    } else if (*nit == "MinType") {
                        pc.MinType = servcfg[*nit].asInt();
                    } else {
                        if (servcfg[*nit].isInt() || servcfg[*nit].isUInt()) {
                            stringstream ss;
                            ss << servcfg[*nit].asInt();
                            pc.others[*nit] = ss.str();
                        } else if (servcfg[*nit].isString()) {
                            pc.others[*nit] = servcfg[*nit].asString();
                        }
                    }
                }
                Ports.push_back(pc);
            }
        } else {
            if (v[*it].isString()) {
                Properties[*it] = v[*it].asString();
            } else if (v[*it].isInt()) {
                stringstream ss;
                ss << v[*it].asInt();
                Properties[*it] = ss.str();
            } else if (v[*it].isObject() || v[*it].isArray()) {
                Properties[*it] = v[*it].toStyledString();
            }
        }
    }


    return 0;
}

int ServerStatus::serializeToJson(Json::Value& v)
{
    v["ID"] = ID;

    for (int i = 0; i < IPs.size(); i++) {
        v["IPs"][i] = IPs[i];
    }

    map<string, string>::iterator it = Properties.begin();
    for (; it != Properties.end(); it++) {
        if (is_digit(it->second)) {
            v[it->first] = atoi(it->second.data());
        } else {
            Json::Reader reader;
            Json::Value tmpv;
            if (reader.parse(it->second, tmpv)) {
                v[it->first] = tmpv;
            } else {
                v[it->first] = it->second;
            }
        }
    }

    vector<PortConfig>::iterator pit = Ports.begin();
    for (int i = 0; pit != Ports.end(); pit++, i++) {
        Json::Value &servcfgs = v["CliConfigs"];
        servcfgs[i]["ListenPort"] = pit->Port;
        servcfgs[i]["MaxType"] = pit->MaxType;
        servcfgs[i]["MinType"] = pit->MinType;
        map<string, string>::iterator oit = pit->others.begin();
        for (; oit != pit->others.end(); oit++) {
            if (is_digit(oit->second)) {
                servcfgs[i][oit->first] = atoi(oit->second.data());
            } else {
                servcfgs[i][oit->first] = oit->second;
            }
        }
    }
}

};