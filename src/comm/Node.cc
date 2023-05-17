#include "Node.hh"

Node::Node(uint16_t _self_conn_id, Config &_config) : self_conn_id(_self_conn_id), config(_config)
{
    // init socket
    sockpp::socket_initializer sock_init;

    // add Controller
    if (self_conn_id != CTRL_NODE_ID)
    {
        connectors_map[CTRL_NODE_ID] = sockpp::tcp_connector();
    }

    // add Agent
    for (auto &item : config.agent_ip_map)
    {
        uint16_t conn_id = item.first;
        if (conn_id != self_conn_id)
        {
            connectors_map[conn_id] = sockpp::tcp_connector();
        }
    }

    // add acceptor
    acceptor = new sockpp::tcp_acceptor(config.port);

    // command handler
    cmd_handler = new CmdHandler(config.num_cmd_handler_thread);
}

Node::~Node()
{
    delete cmd_handler;
    delete acceptor;
}

void Node::start()
{
    // start cmd_handler
    cmd_handler->start();

    // connect all connectors
    connect_all();
}

void Node::stop()
{
    // wait cmd_handler finish
    cmd_handler->wait();

    // disconnect all connectors
    disconnect_all();
}

void Node::connect_all()
{
    unordered_map<uint16_t, thread *> conn_threads;
    // create connect threads
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;
        string ip;
        if (conn_id == CTRL_NODE_ID)
        {
            ip = config.coord_ip;
        }
        else
        {
            ip = config.agent_ip_map[conn_id];
        }

        conn_threads[conn_id] = new thread(&Node::connect_one, this, conn_id, ip, config.port);
    }

    for (auto &item : conn_threads)
    {
        item.second->join();
        delete item.second;
    }

    // create ack threads
    unordered_map<uint16_t, thread *> ack_threads;
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;
        ack_threads[conn_id] = new thread(&Node::ack_one, this, conn_id);
    }

    for (auto &item : ack_threads)
    {
        item.second->join();
        delete item.second;
    }

    printf("Node::test_conn_all successfully connected to %lu nodes\n", connectors_map.size());
}

void Node::connect_one(uint16_t conn_id, string ip, uint16_t port)
{
    // create connection
    sockpp::tcp_connector &connector = connectors_map[conn_id];
    while (!(connector = sockpp::tcp_connector(sockpp::inet_address(ip, port))))
    {
        this_thread::sleep_for(chrono::milliseconds(1));
    }

    // send command
    Command cmd_conn;
    cmd_conn.buildConn(self_conn_id, conn_id);
    connector.write_n(cmd_conn.content, MAX_CMD_LEN * sizeof(unsigned char));
}

void Node::ack_one(uint16_t conn_id)
{
    sockpp::tcp_connector &connector = connectors_map[conn_id];

    // receive ack command
    Command cmd_ack;
    if (connector.read_n(&cmd_ack, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
    {
        fprintf(stderr, "error: invalid read command %u\n", conn_id);
        exit(EXIT_FAILURE);
    }
    cmd_ack.parse();
    if (cmd_ack.type != CommandType::CMD_ACK)
    {
        fprintf(stderr, "error: invalid command type %d from connection %u\n", cmd_ack.type, conn_id);
        exit(EXIT_FAILURE);
    }
}

void Node::disconnect_all()
{
    // create disconnect threads
    unordered_map<uint16_t, thread *> disconnect_threads;
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;
        disconnect_threads[conn_id] = new thread(&Node::disconnect_one, this, conn_id);
    }

    for (auto &item : disconnect_threads)
    {
        item.second->join();
        delete item.second;
    }
}

void Node::disconnect_one(uint16_t conn_id)
{
    sockpp::tcp_connector &connector = connectors_map[conn_id];

    // receive ack command
    Command cmd_disconnect;
    cmd_disconnect.buildConn(self_conn_id, conn_id);
    connector.write_n(cmd_disconnect.content, MAX_CMD_LEN * sizeof(unsigned char));
}
