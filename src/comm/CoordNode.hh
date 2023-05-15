#ifndef __COORD_NODE_HH__
#define __COORD_NODE_HH__

#include "../include/include.hh"
#include "../util/Config.hh"
#include "Node.hh"

class CoordNode
{
private:
    /* data */
public:
    Config &config;

    CoordNode(Config &_config);
    ~CoordNode();
};

#endif // __COORD_NODE_HH__
