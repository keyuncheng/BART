#include "CmdDist.hh"

CmdDist::CmdDist(unsigned int _num_threads) : ThreadPool<Command>(_num_threads)
{
}

CmdDist::~CmdDist()
{
}

void CmdDist::run()
{
    // TO IMPLEMENT
}