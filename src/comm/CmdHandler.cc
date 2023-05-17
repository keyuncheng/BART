#include "CmdHandler.hh"

CmdHandler::CmdHandler(unsigned int _num_threads) : ThreadPool<Command>(_num_threads)
{
}

CmdHandler::~CmdHandler()
{
}

void CmdHandler::run()
{
    // TO IMPLEMENT
}