#include "Config.hh"

Config::Config(string filename)
{
    inipp::Ini<char> ini;
    std::ifstream is(filename);
    ini.parse(is);

    // Common
    unsigned int k_i = 0, m_i = 0, k_f = 0, m_f = 0;
    unsigned int num_stripes = 0;
    unsigned int num_nodes = 0;
    string agent_ips_raw;
    inipp::get_value(ini.sections["Common"], "k_i", k_i);
    inipp::get_value(ini.sections["Common"], "m_i", m_i);
    inipp::get_value(ini.sections["Common"], "k_f", k_f);
    inipp::get_value(ini.sections["Common"], "m_f", m_f);
    inipp::get_value(ini.sections["Common"], "num_nodes", num_nodes);
    inipp::get_value(ini.sections["Common"], "num_stripes", num_stripes);
    inipp::get_value(ini.sections["Common"], "approach", approach);
    inipp::get_value(ini.sections["Common"], "enable_HDFS", enable_HDFS);
    inipp::get_value(ini.sections["Common"], "block_size", block_size);
    inipp::get_value(ini.sections["Common"], "port", port);
    inipp::get_value(ini.sections["Common"], "num_cmd_handler_thread", num_cmd_handler_thread);
    inipp::get_value(ini.sections["Common"], "num_cmd_dist_thread", num_cmd_dist_thread);

    // Controller
    inipp::get_value(ini.sections["Controller"], "ip", coord_ip);
    inipp::get_value(ini.sections["Controller"], "agent_ips", agent_ips_raw);
    inipp::get_value(ini.sections["Controller"], "pre_placement_filename", pre_placement_filename);
    inipp::get_value(ini.sections["Controller"], "pre_block_mapping_filename", pre_block_mapping_filename);
    inipp::get_value(ini.sections["Controller"], "post_placement_filename", post_placement_filename);
    inipp::get_value(ini.sections["Controller"], "post_block_mapping_filename", post_block_mapping_filename);
    inipp::get_value(ini.sections["Controller"], "sg_meta_filename", sg_meta_filename);

    char *raw_str = (char *)malloc(agent_ips_raw.size() * sizeof(char));
    memcpy(raw_str, agent_ips_raw.c_str(), agent_ips_raw.size() * sizeof(char));
    char *token = strtok(raw_str, ",");
    uint16_t aid = 0;
    agent_ip_map[aid] = token;
    aid++;
    while ((token = strtok(NULL, ",")))
    {
        agent_ip_map[aid] = token;
        aid++;
    }

    free(raw_str);

    inipp::get_value(ini.sections["Agent"], "id", agent_id);

    code = ConvertibleCode(k_i, m_i, k_f, m_f);
    settings = ClusterSettings(num_nodes, num_stripes);
}

Config::~Config()
{
}

void Config::print()
{
    printf("========= Config ==========\n");
    printf("Common:\n");
    code.print();
    settings.print();
    printf("Transition approach: %s\n", approach.c_str());
    printf("enable_HDFS: %u\n", enable_HDFS);
    printf("block_size: %u\n", block_size);
    printf("num_cmd_handler_thread: %u\n", num_cmd_handler_thread);
    printf("num_cmd_dist_thread: %u\n", num_cmd_dist_thread);
    printf("===========================\n");

    printf("Controller:\n");
    printf("ip: %s:%u\n", coord_ip.c_str(), port);
    printf("pre_placement_filename: %s\n", pre_placement_filename.c_str());
    printf("pre_block_mapping_filename: %s\n", pre_block_mapping_filename.c_str());
    printf("post_placement_filename: %s\n", post_placement_filename.c_str());
    printf("post_block_mapping_filename: %s\n", post_block_mapping_filename.c_str());
    printf("sg_meta_filename: %s\n", sg_meta_filename.c_str());
    printf("===========================\n");

    printf("Agent (%lu):\n", agent_ip_map.size());
    for (auto &item : agent_ip_map)
    {
        string is_current;
        if (agent_id == item.first)
        {
            is_current = " (current)";
        }
        printf("Agent %u%s, ip: %s:%d\n", item.first, is_current.c_str(), item.second.c_str(), port);
    }
    printf("===========================\n");
}