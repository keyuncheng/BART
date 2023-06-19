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
    string controller_addr_raw;
    inipp::get_value(ini.sections["Controller"], "controller_addr", controller_addr_raw);
    inipp::get_value(ini.sections["Controller"], "agent_addrs", agent_addrs_raw);
    inipp::get_value(ini.sections["Controller"], "pre_placement_filename", pre_placement_filename);
    inipp::get_value(ini.sections["Controller"], "pre_block_mapping_filename", pre_block_mapping_filename);
    inipp::get_value(ini.sections["Controller"], "post_placement_filename", post_placement_filename);
    inipp::get_value(ini.sections["Controller"], "post_block_mapping_filename", post_block_mapping_filename);

    // controller ip, ports
    std::vector<std::string> controller_parts;
    std::istringstream iss(controller_addr_raw);
    string controller_token;
    while (std::getline(iss, controller_token, ':'))
    {
        controller_parts.push_back(controller_token);
    }
    string controller_ip = controller_parts[0];
    unsigned int controller_cmd_port = stoul(controller_parts[1]);
    unsigned int controller_data_port = stoul(controller_parts[2]);
    controller_addr = tuple<string, unsigned int, unsigned int>(controller_ip, controller_cmd_port, controller_data_port);

    // agent ip, port
    char *raw_str = (char *)malloc(agent_addrs_raw.size() * sizeof(char));
    memcpy(raw_str, agent_addrs_raw.c_str(), agent_addrs_raw.size() * sizeof(char));

    char *token = strtok(raw_str, ",");
    string agent_addr(token);

    std::vector<std::string> agent_parts;
    std::istringstream iss_agent(agent_addr);
    string agent_token;
    while (std::getline(iss_agent, agent_token, ':'))
    {
        agent_parts.push_back(agent_token);
    }
    string agent_ip = agent_parts[0];
    unsigned int agent_cmd_port = stoul(agent_parts[1]);
    unsigned int agent_data_port = stoul(agent_parts[2]);
    uint16_t aid = 0;
    agent_addr_map[aid] = tuple<string, unsigned int, unsigned int>(agent_ip, agent_cmd_port, agent_data_port);
    aid++;

    while ((token = strtok(NULL, ",")))
    {
        string agent_addr(token);
        std::vector<std::string> agent_parts;
        string agent_token;
        std::istringstream iss_agent(agent_addr);
        while (std::getline(iss_agent, agent_token, ':'))
        {
            agent_parts.push_back(agent_token);
        }
        agent_ip = agent_parts[0];
        agent_cmd_port = stoul(agent_parts[1]);
        agent_data_port = stoul(agent_parts[2]);
        agent_addr_map[aid] = tuple<string, unsigned int, unsigned int>(agent_ip, agent_cmd_port, agent_data_port);
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
    printf("block_size: %lu\n", block_size);
    printf("num_cmd_handler_thread: %u\n", num_cmd_handler_thread);
    printf("num_cmd_dist_thread: %u\n", num_cmd_dist_thread);
    printf("===========================\n");

    printf("Controller:\n");
    printf("addr: %s:%u and %s:%u\n", std::get<0>(controller_addr).c_str(), std::get<1>(controller_addr), std::get<0>(controller_addr).c_str(), std::get<2>(controller_addr));
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
        printf("Agent %u%s, command ip: %s:%d, data ip: %s:%d\n", item.first, is_current.c_str(), std::get<0>(agent_addr).c_str(), std::get<1>(agent_addr), std::get<0>(agent_addr).c_str(), std::get<2>(agent_addr));
    }
    printf("===========================\n");
}