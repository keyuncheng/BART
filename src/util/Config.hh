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
    string approach; // transitioning approach

    // Controller
    pair<string, unsigned int> controller_addr;
    map<uint16_t, pair<string, unsigned int>> agent_addr_map;
    string pre_placement_filename;      // pre-transition placement
    string pre_block_mapping_filename;  // pre-transition block mapping
    string post_placement_filename;     // post-transition placement
    string post_block_mapping_filename; // post-transition block mapping
    string sg_meta_filename;            // stripe group metadata

    // Agent
    uint64_t block_size;              // block size in Bytes
    unsigned int num_compute_workers; // number of compute workers
    unsigned int num_reloc_workers;   // number of relocation workers

    Config(string filename);
    ~Config();

    void print();
};

#endif // __CONFIG_HH__