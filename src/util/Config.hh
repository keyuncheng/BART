#ifndef __CONFIG_HH__
#define __CONFIG_HH__

#include "../include/include.hh"
#include "../model/ConvertibleCode.hh"
#include "../model/ClusterSettings.hh"
#include "inipp.h"

class Config
{
private:
public:
    string filename;
    ConvertibleCode code;
    ClusterSettings settings;
    in_port_t port;

    // Coordinator
    string coord_ip;
    map<uint16_t, string> agent_ip_map;

    // Agent
    uint16_t agent_id;

    Config(string filename);
    ~Config();

    void print();
};

#endif // __CONFIG_HH__