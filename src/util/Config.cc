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

    code = ConvertibleCode(k_i, m_i, k_f, m_f);
    settings = ClusterSettings(num_nodes, num_stripes);

    // Controller
    string controller_addr_raw;
    inipp::get_value(ini.sections["Controller"], "controller_addr", controller_addr_raw);
    inipp::get_value(ini.sections["Controller"], "agent_addrs", agent_addrs_raw);
    inipp::get_value(ini.sections["Controller"], "pre_placement_filename", pre_placement_filename);
    inipp::get_value(ini.sections["Controller"], "pre_block_mapping_filename", pre_block_mapping_filename);
    inipp::get_value(ini.sections["Controller"], "post_placement_filename", post_placement_filename);
    inipp::get_value(ini.sections["Controller"], "post_block_mapping_filename", post_block_mapping_filename);
    inipp::get_value(ini.sections["Controller"], "sg_meta_filename", sg_meta_filename);

    // controller ip, port
    auto delim_pos = controller_addr_raw.find(":");
    string controller_ip = controller_addr_raw.substr(0, delim_pos);
    unsigned int controller_port = stoul(controller_addr_raw.substr(delim_pos + 1, controller_addr_raw.size() - controller_ip.size() - 1).c_str());
    controller_addr = pair<string, unsigned int>(controller_ip, controller_port);

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

    // Agent
    inipp::get_value(ini.sections["Agent"], "block_size", block_size);
    inipp::get_value(ini.sections["Agent"], "num_compute_workers", num_compute_workers);
    inipp::get_value(ini.sections["Agent"], "num_reloc_workers", num_reloc_workers);
}

Config::~Config()
{
}

void Config::print()
{
    printf("========= Configurations ==========\n");

    printf("========= Common ==========\n");
    code.print();
    settings.print();
    printf("transitioning approach: %s\n", approach.c_str());
    printf("===========================\n");

    printf("========= Controller ==========\n");
    printf("address: %s:%u\n", controller_addr.first.c_str(), controller_addr.second);
    printf("pre_placement_filename: %s\n", pre_placement_filename.c_str());
    printf("pre_block_mapping_filename: %s\n", pre_block_mapping_filename.c_str());
    printf("post_placement_filename: %s\n", post_placement_filename.c_str());
    printf("post_block_mapping_filename: %s\n", post_block_mapping_filename.c_str());
    printf("sg_meta_filename: %s\n", sg_meta_filename.c_str());
    printf("===========================\n");

    printf("========= Agents ==========\n");
    printf("block_size: %lu\n", block_size);
    printf("num_compute_workers: %u\n", num_compute_workers);
    printf("num_reloc_workers: %u\n", num_reloc_workers);
    printf("addresses: (%lu)\n", agent_addr_map.size());
    for (auto &item : agent_addr_map)
    {
        auto &agent_addr = item.second;
        printf("Agent %u, ip: %s:%d\n", item.first, agent_addr.first.c_str(), agent_addr.second);
    }
    printf("===========================\n");
}