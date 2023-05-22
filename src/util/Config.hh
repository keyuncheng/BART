#ifndef __CONFIG_HH__
#define __CONFIG_HH__

#include "../include/include.hh"
#include "../model/ConvertibleCode.hh"
#include "../model/ClusterSettings.hh"
#include "../util/inipp.h"

class Config
{
private:
public:
    // Common
    ConvertibleCode code;
    ClusterSettings settings;
    string approach;
    bool enable_HDFS;
    unsigned int block_size; // block size in Bytes
    uint16_t port;
    unsigned int num_cmd_handler_thread;
    unsigned int num_cmd_dist_thread;

    // Controller
    string coord_ip;
    map<uint16_t, string> agent_ip_map;
    string placement_filename;
    string block_mapping_filename;

    // Agent
    uint16_t agent_id;

    Config(string filename);
    ~Config();

    void print();
};

#endif // __CONFIG_HH__