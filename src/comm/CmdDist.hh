#ifndef __CMD_DIST_HH__
#define __CMD_DIST_HH__

#include "../include/include.hh"
#include "Command.hh"
#include "../util/ThreadPool.hh"

class CmdDist : public ThreadPool<Command>
{
private:
    /* data */
public:
    CmdDist(unsigned int _num_threads);
    ~CmdDist();

    void run() override;
};

#endif // __CMD_DIST_HH__