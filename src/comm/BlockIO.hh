#ifndef __BLOCK_IO_HH__
#define __BLOCK_IO_HH__

#include "../include/include.hh"
#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_socket.h"

#include <iostream>
#include <fstream>
#include <cstdio>

class BlockIO
{
private:
    /* data */
public:
    BlockIO(/* args */);
    ~BlockIO();

    static uint64_t readBlock(string block_path, unsigned char *buffer, uint64_t block_size);
    static uint64_t writeBlock(string block_path, unsigned char *buffer, uint64_t block_size);
    static void deleteBlock(string block_path);

    static uint64_t sendBlock(sockpp::tcp_connector &connector, unsigned char *buffer, uint64_t block_size);
    static uint64_t recvBlock(sockpp::tcp_socket &skt, unsigned char *buffer, uint64_t block_size);
};

#endif // __BLOCK_IO_HH__