#ifndef __RELOC_WORKER_HH__
#define __RELOC_WORKER_HH__

#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_acceptor.h"

#include "../include/include.hh"
#include "../util/ThreadPool.hh"
#include "../util/Config.hh"
#include "../util/MultiWriterQueue.h"
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

    RelocWorker(Config &_config, unsigned int _self_worker_id, uint16_t _self_conn_id, MultiWriterQueue<Command> &_reloc_task_queue);
    ~RelocWorker();

    /**
     * @brief thread initializer
     *
     */
    void run() override;
};

#endif // __RELOC_WORKER_HH__
