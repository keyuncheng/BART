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
    string agent_addrs_raw;
    inipp::get_value(ini.sections["Common"], "k_i", k_i);
    inipp::get_value(ini.sections["Common"], "m_i", m_i);
    inipp::get_value(ini.sections["Common"], "k_f", k_f);
    inipp::get_value(ini.sections["Common"], "m_f", m_f);
    inipp::get_value(ini.sections["Common"], "num_nodes", num_nodes);
    inipp::get_value(ini.sections["Common"], "num_stripes", num_stripes);
    inipp::get_value(ini.sections["Common"], "approach", approach);
    inipp::get_value(ini.sections["Common"], "enable_HDFS", enable_HDFS);
    inipp::get_value(ini.sections["Common"], "block_size", block_size);
    inipp::get_value(ini.sections["Common"], "num_cmd_handler_thread", num_cmd_handler_thread);
    inipp::get_value(ini.sections["Common"], "num_cmd_dist_thread", num_cmd_dist_thread);

    // Controller
    string controller_addr;
    inipp::get_value(ini.sections["Controller"], "controller_addr", controller_addr);
    inipp::get_value(ini.sections["Controller"], "agent_addrs", agent_addrs_raw);
    inipp::get_value(ini.sections["Controller"], "pre_placement_filename", pre_placement_filename);
    inipp::get_value(ini.sections["Controller"], "pre_block_mapping_filename", pre_block_mapping_filename);
    inipp::get_value(ini.sections["Controller"], "post_placement_filename", post_placement_filename);
    inipp::get_value(ini.sections["Controller"], "post_block_mapping_filename", post_block_mapping_filename);
    inipp::get_value(ini.sections["Controller"], "sg_meta_filename", sg_meta_filename);

    // controller ip, port
    auto delim_pos = controller_addr.find(":");
    controller_ip = controller_addr.substr(0, delim_pos);
    controller_port = stoul(controller_addr.substr(delim_pos + 1, controller_addr.size() - controller_ip.size() - 1).c_str());

    // agent ip, port
    char *raw_str = (char *)malloc(agent_addrs_raw.size() * sizeof(char));
    memcpy(raw_str, agent_addrs_raw.c_str(), agent_addrs_raw.size() * sizeof(char));

    char *token = strtok(raw_str, ",");
    string agent_addr(token);
    delim_pos = agent_addr.find(":");
    string agent_ip = agent_addr.substr(0, delim_pos);
    unsigned int agent_port = stoul(agent_addr.substr(delim_pos + 1, agent_addr.size() - agent_ip.size() - 1).c_str());
    uint16_t aid = 0;
    agent_addr_map[aid] = pair<string, unsigned int>(agent_ip, agent_port);
    aid++;

    while ((token = strtok(NULL, ",")))
    {
        agent_addr = token;
        delim_pos = agent_addr.find(":");
        agent_ip = agent_addr.substr(0, delim_pos);
        agent_port = stoul(agent_addr.substr(delim_pos + 1, agent_addr.size() - agent_ip.size() - 1).c_str());
        agent_addr_map[aid] = pair<string, unsigned int>(agent_ip, agent_port);
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
    printf("addr: %s:%u\n", controller_ip.c_str(), controller_port);
    printf("pre_placement_filename: %s\n", pre_placement_filename.c_str());
    printf("pre_block_mapping_filename: %s\n", pre_block_mapping_filename.c_str());
    printf("post_placement_filename: %s\n", post_placement_filename.c_str());
    printf("post_block_mapping_filename: %s\n", post_block_mapping_filename.c_str());
    printf("sg_meta_filename: %s\n", sg_meta_filename.c_str());
    printf("===========================\n");

    printf("Agents (%lu):\n", agent_addr_map.size());
    for (auto &item : agent_addr_map)
    {
        auto &agent_addr = item.second;
        string is_current;
        if (agent_id == item.first)
        {
            is_current = " (current)";
        }
        printf("Agent %u%s, ip: %s:%d\n", item.first, is_current.c_str(), agent_addr.first.c_str(), agent_addr.second);
    }
    printf("===========================\n");
}