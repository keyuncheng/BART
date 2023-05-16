#include "Config.hh"

Config::Config(string filename)
{
    inipp::Ini<char> ini;
    std::ifstream is(filename);
    ini.parse(is);

    // Common
    uint8_t k_i = 0, m_i = 0, k_f = 0, m_f = 0;
    uint32_t num_stripes = 0;
    uint16_t num_nodes = 0;
    string agent_ips_raw;

    inipp::get_value(ini.sections["Common"], "k_i", k_i);
    inipp::get_value(ini.sections["Common"], "m_i", m_i);
    inipp::get_value(ini.sections["Common"], "k_f", k_f);
    inipp::get_value(ini.sections["Common"], "m_f", m_f);
    inipp::get_value(ini.sections["Common"], "num_nodes", num_nodes);
    inipp::get_value(ini.sections["Common"], "num_stripes", num_stripes);
    inipp::get_value(ini.sections["Common"], "port", port);

    inipp::get_value(ini.sections["Coordinator"], "ip", coord_ip);
    inipp::get_value(ini.sections["Coordinator"], "agent_ips", agent_ips_raw);

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
    code.print();
    settings.print();

    printf("Coordinator:\n");
    printf("ip: %s:%u\n", coord_ip.c_str(), port);
    printf("Agent ips (%lu):\n", agent_ip_map.size());
    for (auto &item : agent_ip_map)
    {
        printf("Agent %u, ip: %s\n", item.first, item.second.c_str());
    }

    printf("Agent (current):\n");
    printf("id: %u, ip: %s:%u\n", agent_id, agent_ip_map[agent_id].c_str(), port);
}