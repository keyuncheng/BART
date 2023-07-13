#ifndef __CLUSTER_SETTINGS_HH__
#define __CLUSTER_SETTINGS_HH__

#include "../include/include.hh"
#include "ConvertibleCode.hh"

class ClusterSettings
{
private:
    /* data */
public:
    uint16_t num_nodes;
    uint32_t num_stripes;

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
};

#endif // __CLUSTER_SETTINGS_HH__