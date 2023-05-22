#include "ClusterSettings.hh"

ClusterSettings::ClusterSettings()
{
}

ClusterSettings::ClusterSettings(uint16_t _num_nodes, uint32_t _num_stripes) : num_nodes(_num_nodes), num_stripes(_num_stripes)
{
}

ClusterSettings::~ClusterSettings()
{
}

void ClusterSettings::print()
{
    printf("ClusterSettings: num_nodes: %u, num_stripes: %u\n", num_nodes, num_stripes);
}

bool ClusterSettings::isParamValid(const ConvertibleCode &code)
{
    if (code.k_i == 0 || code.m_i == 0 || code.k_f == 0 || code.m_f == 0)
    {
        return false;
    }

    if (num_nodes == 0 || num_stripes == 0)
    {
        return false;
    }

    if (num_stripes % code.lambda_i != 0)
    {
        return false;
    }

    return true;
}