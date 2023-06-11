#include "include/include.hh"
#include "util/Config.hh"
#include "comm/AgentNode.hh"

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("usage: ./Agent agent_id config_filename\n");
        return -1;
    }

    uint16_t agent_id = atoi(argv[1]);
    string config_filename = argv[2];

    Config config(config_filename);
    config.agent_id = agent_id; // now set as cmd line input
    config.print();

    AgentNode agent_node(config.agent_id, config);

    // benchmarking
    double finish_time = 0.0;
    struct timeval start_time, end_time;
    gettimeofday(&start_time, nullptr);

    agent_node.start();

    agent_node.stop();

    gettimeofday(&end_time, nullptr);
    finish_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                  (end_time.tv_usec - start_time.tv_usec) / 1000;
    printf("Controller::main finished transition, time: %f ms\n", finish_time);
}