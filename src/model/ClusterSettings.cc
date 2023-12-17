#include "ClusterSettings.hh"

ClusterSettings::ClusterSettings()
{
}

ClusterSettings::ClusterSettings(uint16_t _num_nodes, uint32_t _num_stripes) : num_nodes(_num_nodes), num_stripes(_num_stripes), is_heterogeneous(false)
{
}

bool ClusterSettings::loadBWProfile(string bw_filename)
{
    // read bandwidth profile from bw_filename
    ifstream ifs(bw_filename.c_str());

    if (ifs.fail())
    {
        printf("invalid bandwidth file: %s\n", bw_filename.c_str());
        return false;
    }

    is_heterogeneous = true;
    bw_profile.upload.resize(num_nodes);
    bw_profile.download.resize(num_nodes);

    string line;
    bool is_upload = true;
    while (getline(ifs, line))
    {
        // TODO
        istringstream iss(line);
        for (uint8_t node_id = 0; node_id < num_nodes; node_id++)
        {
            double bw_node; // MB/s
            iss >> bw_node;
            if (is_upload == true)
            {
                bw_profile.upload[node_id] = bw_node;
            }
            else
            {
                bw_profile.download[node_id] = bw_node;
            }
        }

        is_upload = false;
    }

    ifs.close();

    printf("finished loading bandwidth profile from %s\n", bw_filename.c_str());

    return true;
}

ClusterSettings::~ClusterSettings()
{
}

void ClusterSettings::print()
{
    printf("ClusterSettings: num_nodes: %u, num_stripes: %u\n", num_nodes, num_stripes);

    if (is_heterogeneous == true)
    {
        printf("bw_profile:\n");
        Utils::printVector(bw_profile.upload);
        Utils::printVector(bw_profile.download);
    }
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