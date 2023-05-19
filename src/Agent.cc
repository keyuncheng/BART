#include "include/include.hh"
#include "util/Config.hh"
#include "comm/AgentNode.hh"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("usage: ./Agent config_filename");
        return -1;
    }

    string config_filename = argv[1];

    Config config(config_filename);
    config.print();

    AgentNode agent_node(config.agent_id, config);

    agent_node.start();
}