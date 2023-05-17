#ifndef __CMD_HANDLER_HH__
#define __CMD_HANDLER_HH__

#include "../include/include.hh"
#include "../util/ThreadPool.hh"
#include "Command.hh"

class CmdHandler : public ThreadPool<Command>
{
private:
    /* data */
public:
    CmdHandler(unsigned int _num_threads);
    ~CmdHandler();

    void run() override;
};

#endif // __CMD_HANDLER_HH__