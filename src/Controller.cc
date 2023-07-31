#include "include/include.hh"
#include "util/Config.hh"
#include "comm/CtrlNode.hh"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("usage: ./Controller config_filename\n");
        return -1;
    }

    string config_filename = argv[1];

    Config config(config_filename);
    config.print();

    CtrlNode ctrl_node(CTRL_NODE_ID, config);

    // benchmarking
    double finish_time = 0.0;
    struct timeval start_time, end_time;
    gettimeofday(&start_time, nullptr);

    ctrl_node.start();

    // generate transition solution
    ctrl_node.genTransSolution();

    ctrl_node.stop();

    gettimeofday(&end_time, nullptr);
    finish_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                  (end_time.tv_usec - start_time.tv_usec) / 1000;
    printf("Controller::main finished transitioning, time: %f ms\n", finish_time);
}