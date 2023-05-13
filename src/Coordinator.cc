#include "include/include.hh"
#include "util/Config.hh"
#include "CoordNode.hh"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("usage: ./Coordinator config_filename");
        return -1;
    }

    string config_filename = argv[1];

    Config config(config_filename);
    config.print();

    CoordNode coord_node;
}