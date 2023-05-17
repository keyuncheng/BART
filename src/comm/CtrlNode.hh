#ifndef __CTRL_NODE_HH__
#define __CTRL_NODE_HH__

#include "../include/include.hh"
#include "../util/Config.hh"
#include "Node.hh"

class CtrlNode
{
private:
    /* data */
public:
    Config &config;

    CtrlNode(Config &_config);
    ~CtrlNode();
};

#endif // __CTRL_NODE_HH__
