#ifndef __RELOC_WORKER_HH__
#define __RELOC_WORKER_HH__

#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_acceptor.h"

#include "../include/include.hh"
#include "../util/ThreadPool.hh"
#include "../util/Config.hh"
#include "../util/MultiWriterQueue.h"
#include "../util/MemoryPool.hh"
#include "Command.hh"
#include "Node.hh"
#include "BlockIO.hh"

class RelocWorker : public ThreadPool
{
private:
    /* data */
public:
    // config
    Config &config;

    // current worker id
    unsigned int self_worker_id;

    // current connection id
    uint16_t self_conn_id;

    // relocation task queue
    MultiWriterQueue<Command> &reloc_task_queue;

    // sockets for data commands distribution
    unordered_map<uint16_t, pair<string, unsigned int>> data_cmd_addrs_map;
    unordered_map<uint16_t, sockpp::tcp_connector> data_cmd_connectors_map;
    unordered_map<uint16_t, sockpp::tcp_socket> data_cmd_sockets_map;
    unsigned int data_cmd_acceptor_port;
    sockpp::tcp_acceptor *data_cmd_acceptor;

    // sockets for data content distribution
    unordered_map<uint16_t, pair<string, unsigned int>> data_content_addrs_map;
    unordered_map<uint16_t, sockpp::tcp_connector> data_content_connectors_map;
    unordered_map<uint16_t, sockpp::tcp_socket> data_content_sockets_map;
    unsigned int data_content_acceptor_port;
    sockpp::tcp_acceptor *data_content_acceptor;

    // data handler threads
    unordered_map<uint16_t, thread *> data_handler_threads_map;

    // memory pool
    MemoryPool *memory_pool;

    RelocWorker(Config &_config, unsigned int _self_worker_id, uint16_t _self_conn_id, MultiWriterQueue<Command> &_reloc_task_queue);
    ~RelocWorker();

    /**
     * @brief thread initializer
     *
     */
    void run() override;

    // send data thread
    void sendDataToAgent(Command *reloc_cmd, uint16_t dst_node_id);

    // data transfer thread handler
    void handleDataTransfer(uint16_t src_conn_id);

    // detach write
    void writeBlockToDisk(string block_path, unsigned char *data_buffer, uint64_t block_size);
};

#endif // __RELOC_WORKER_HH__
