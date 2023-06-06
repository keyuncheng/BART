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
    unsigned int num_cmd_handler_thread;
    unsigned int num_cmd_dist_thread;

    // Controller
    string controller_ip;
    unsigned int controller_port;
    map<uint16_t, pair<string, unsigned int>> agent_addr_map;
    string pre_placement_filename;      // pre-transition placement
    string pre_block_mapping_filename;  // pre-transition block mapping
    string post_placement_filename;     // post-transition placement
    string post_block_mapping_filename; // post-transition block mapping
    string sg_meta_filename;            // stripe group metadata

    // Agent
    uint16_t agent_id;

    Config(string filename);
    ~Config();

    void print();
};

#endif // __CONFIG_HH__