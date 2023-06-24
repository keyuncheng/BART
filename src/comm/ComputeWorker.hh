#ifndef __COMPUTE_WORKER_HH__
#define __COMPUTE_WORKER_HH__

#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_acceptor.h"
#include <isa-l.h>

#include "../include/include.hh"
#include "../util/ThreadPool.hh"
#include "../util/Config.hh"
#include "../util/MessageQueue.hh"
#include "../util/MultiWriterQueue.h"
#include "ParityComputeTask.hh"
#include "Command.hh"
#include "Node.hh"
#include "BlockIO.hh"

class ComputeWorker : public ThreadPool
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

    // compute task queue: retrieves computation task from Controller, and pass to a ComputeWorker (CmdHandler -> ComputeWorker)
    MessageQueue<ParityComputeTask> &compute_task_queue;

    // block relocation task queue: each retrieves block relocation task (from both CmdHandler and ComputeWorker), and pass to a RelocWorker (ComputerWorker / CmdHandler -> RelocWorker<worker_id>)
    unordered_map<unsigned int, MultiWriterQueue<Command> *> &reloc_task_queues;

    // coding matrix
    unsigned char *re_matrix;
    unsigned char *re_encode_gftbl;

    unsigned char **pm_matrix;
    unsigned char **pm_encode_gftbl;

    // block buffers for re-encoding and parity merging
    unsigned char *block_buffer;
    unsigned char **re_buffers;
    unsigned char **pm_buffers;

    // sockets for data commands distribution
    unordered_map<uint16_t, pair<string, unsigned int>> data_cmd_addrs_map;
    unordered_map<uint16_t, sockpp::tcp_connector> data_cmd_connectors_map;
    unordered_map<uint16_t, sockpp::tcp_socket> data_cmd_sockets_map;
    sockpp::tcp_acceptor *data_cmd_acceptor;

    // sockets for data content distribution
    unordered_map<uint16_t, pair<string, unsigned int>> data_content_addrs_map;
    unordered_map<uint16_t, sockpp::tcp_connector> data_content_connectors_map;
    unordered_map<uint16_t, sockpp::tcp_socket> data_content_sockets_map;
    sockpp::tcp_acceptor *data_content_acceptor;

    // data handler threads
    unordered_map<uint16_t, thread *> data_handler_threads_map;

    ComputeWorker(Config &_config, unsigned int _self_worker_id, uint16_t _self_conn_id, MessageQueue<ParityComputeTask> &_compute_task_queue, unordered_map<unsigned int, MultiWriterQueue<Command> *> &_reloc_task_queues);
    ~ComputeWorker();

    void run() override;

    // data request thread
    void requestDataFromAgent(ParityComputeTask *parity_compute_task, uint16_t src_node_id, vector<unsigned char *> data_buffers);

    // data transfer thread handler
    void handleDataTransfer(uint16_t src_conn_id);

    // erasure coding initializer
    void initECTables();
    void destroyECTables();
    unsigned char gfPow(unsigned char val, unsigned int times);
};

#endif // __COMPUTE_WORKER_HH__