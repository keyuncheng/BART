#ifndef __BLOCK_REQ_HANDLER_HH__
#define __BLOCK_REQ_HANDLER_HH__

#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_acceptor.h"

#include "../include/include.hh"

class BlockReqHandler
{
private:
    /* data */
public:
    BlockReqHandler(/* args */);
    ~BlockReqHandler();

    sockpp::tcp_acceptor *acceptor;
};

#endif // __BLOCK_REQ_HANDLER_HH__
