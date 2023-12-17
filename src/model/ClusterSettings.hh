#ifndef __CLUSTER_SETTINGS_HH__
#define __CLUSTER_SETTINGS_HH__

#include "../include/include.hh"
#include "ConvertibleCode.hh"

typedef struct BWProfile
{
    vector<double> upload;   // in MB/s
    vector<double> download; // in MB/s
} BWProfile;

class ClusterSettings
{
private:
    /* data */
public:
    uint16_t num_nodes;
    uint32_t num_stripes;

    // bandwidth profile
    bool is_heterogeneous;
    BWProfile bw_profile;

    ClusterSettings();
    ClusterSettings(uint16_t _num_nodes, uint32_t _num_stripes);
    ~ClusterSettings();

    /**
     * @brief print cluster settings
     *
     */
    void print();

    /**
     * @brief check if the parameters are valid
     *
     * @param code
     * @return true
     * @return false
     */
    bool isParamValid(const ConvertibleCode &code);

    /**
     * @brief load bandwidth profile (heterogeneous network settings)
     *
     * @param bw_filename
     * @return true
     * @return false
     */
    bool loadBWProfile(string bw_filename);
};

#endif // __CLUSTER_SETTINGS_HH__