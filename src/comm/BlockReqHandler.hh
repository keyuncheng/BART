#ifndef __BLOCK_REQ_HANDLER_HH__
#define __BLOCK_REQ_HANDLER_HH__

#include <mutex>
#include <condition_variable>

#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_acceptor.h"

#include "../include/include.hh"
#include "../util/Config.hh"
#include "Command.hh"
#include "../util/ThreadPool.hh"
#include "../util/MessageQueue.hh"
#include "BlockIO.hh"

typedef struct BlockReq
{
    Command cmd;
    sockpp::tcp_socket *skt;
} BlockReq;

class BlockReqHandler : public ThreadPool
{
private:
    /* data */
public:
    // config
    Config &config;

    // current connection id
    uint16_t self_conn_id;

    // number of workers
    unsigned int num_workers;

    mutex max_conn_mutex;
    std::condition_variable max_conn_cv;
    unsigned int cur_conn;
    unsigned int max_conn;

    sockpp::tcp_acceptor *acceptor;

    // message queues for request handlers
    unordered_map<unsigned int, MessageQueue<BlockReq> *> req_handler_queues;

    // request handler threads
    unordered_map<uint16_t, thread *> req_handler_threads_map;

    BlockReqHandler(Config &_config, uint16_t _self_conn_id, unsigned int _num_workers);
    ~BlockReqHandler();

    /**
     * @brief thread initializer
     *
     */
    void run() override;

    // block request handler
    void handleBlockRequest(unsigned int self_worker_id);

    // stop handling for the main thread
    void stopHandling();
};

#endif // __BLOCK_REQ_HANDLER_HH__
