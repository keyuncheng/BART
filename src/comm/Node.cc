#include "Node.hh"

Node::Node(uint16_t _self_conn_id, Config &_config) : self_conn_id(_self_conn_id), config(_config)
{
    // init socket
    sockpp::socket_initializer::initialize();

    if (self_conn_id != CTRL_NODE_ID)
    {
        // add controller
        connectors_map[CTRL_NODE_ID] = sockpp::tcp_connector();
        sockets_map[CTRL_NODE_ID] = sockpp::tcp_socket();
    }

    // add Agent
    for (auto &item : config.agent_addr_map)
    {
        uint16_t conn_id = item.first;
        if (self_conn_id != conn_id)
        {
            connectors_map[conn_id] = sockpp::tcp_connector();
            sockets_map[conn_id] = sockpp::tcp_socket();
        }
    }

    // add acceptor
    unsigned int self_port = 0;
    if (self_conn_id == CTRL_NODE_ID)
    {
        self_port = config.controller_addr.second;
    }
    else
    {
        self_port = config.agent_addr_map[self_conn_id].second;
    }

    acceptor = new sockpp::tcp_acceptor(self_port);

    printf("Node %u: start connection\n", self_conn_id);

    // create ack connector threads
    thread ack_conn_thread([&]
                           { Node::ackConnAll(); });

    // connect all nodes
    connectAll();

    // join ack connector threads
    ack_conn_thread.join();
}

Node::~Node()
{
    acceptor->close();
    delete acceptor;
}

void Node::connectAll()
{
    unordered_map<uint16_t, thread *> conn_threads;
    // create connect threads
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;
        string ip;
        unsigned int port;
        if (conn_id == CTRL_NODE_ID)
        {
            ip = config.controller_addr.first;
            port = config.controller_addr.second;
        }
        else
        {
            ip = config.agent_addr_map[conn_id].first;
            port = config.agent_addr_map[conn_id].second;
        }

        conn_threads[conn_id] = new thread(&Node::connectOne, this, conn_id, ip, port);
    }

    for (auto &item : conn_threads)
    {
        item.second->join();
        delete item.second;
    }

    // create handle ack threads
    unordered_map<uint16_t, thread *> ack_threads;
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;
        ack_threads[conn_id] = new thread(&Node::handleAckOne, this, conn_id);
    }

    for (auto &item : ack_threads)
    {
        item.second->join();
        delete item.second;
    }

    printf("Node %u: successfully connected to %lu nodes\n", self_conn_id, connectors_map.size());
}

void Node::connectOne(uint16_t conn_id, string ip, uint16_t port)
{
    // create connection
    sockpp::tcp_connector &connector = connectors_map[conn_id];
    while (!(connector = sockpp::tcp_connector(sockpp::inet_address(ip, port))))
    {
        this_thread::sleep_for(chrono::milliseconds(1));
    }

    // printf("Successfully created connection to conn_id: %u\n", conn_id);

    // send the connection command
    Command cmd_conn;
    cmd_conn.buildCommand(CommandType::CMD_CONN, self_conn_id, conn_id);

    // printf("Node %u connected to Node %u: \n", self_conn_id, conn_id);
    // Utils::printUCharBuffer(cmd_conn.content, MAX_CMD_LEN);

    if (connector.write_n(cmd_conn.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
    {
        fprintf(stderr, "error send cmd_conn\n");
        exit(EXIT_FAILURE);
    }
}

void Node::ackConnAll()
{
    uint16_t num_acked_nodes = 0;
    uint16_t num_conns = connectors_map.size();
    while (num_acked_nodes < num_conns)
    {
        sockpp::inet_address conn_addr;
        sockpp::tcp_socket skt = acceptor->accept(&conn_addr);

        if (!skt)
        {
            fprintf(stderr, "invalid socket: %s\n", acceptor->last_error_str().c_str());
            exit(EXIT_FAILURE);
        }

        // parse the connection command
        Command cmd_conn;
        if (skt.read_n(cmd_conn.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
        {
            fprintf(stderr, "error reading cmd_conn\n");
            exit(EXIT_FAILURE);
        }

        // printf("Received at Node %u: \n", self_conn_id);
        // Utils::printUCharBuffer(cmd_conn.content, MAX_CMD_LEN);

        cmd_conn.parse();

        if (cmd_conn.type != CommandType::CMD_CONN || cmd_conn.dst_conn_id != self_conn_id)
        {
            fprintf(stderr, "invalid cmd_conn: type: %u, dst_conn_id: %u\n", cmd_conn.type, cmd_conn.dst_conn_id);
            exit(EXIT_FAILURE);
        }

        // maintain the socket
        uint16_t conn_id = cmd_conn.src_conn_id;
        sockets_map[conn_id] = move(skt);

        // send the ack command
        auto &connector = sockets_map[conn_id];
        Command cmd_ack;
        cmd_ack.buildCommand(CommandType::CMD_ACK, self_conn_id, conn_id);

        // printf("Reply ack at Node %u -> %u \n", self_conn_id, conn_id);
        // Utils::printUCharBuffer(cmd_ack.content, MAX_CMD_LEN);

        if (connector.write_n(cmd_ack.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
        {
            fprintf(stderr, "error send cmd_ack\n");
            exit(EXIT_FAILURE);
        }

        num_acked_nodes++;

        // printf("Node %u: received connection and acked to Node %u; connected to (%u / %u) Nodes\n", self_conn_id, conn_id, num_acked_nodes, num_conns);
    }
}

void Node::handleAckOne(uint16_t conn_id)
{
    sockpp::tcp_connector &connector = connectors_map[conn_id];

    // parse the ack command
    Command cmd_ack;
    if (connector.read_n(cmd_ack.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
    {
        fprintf(stderr, "error reading cmd_ack from %u\n", conn_id);
        exit(EXIT_FAILURE);
    }
    cmd_ack.parse();

    // printf("Handle ack at Node %u -> %u \n", self_conn_id, conn_id);
    // Utils::printUCharBuffer(cmd_ack.content, MAX_CMD_LEN);

    if (cmd_ack.type != CommandType::CMD_ACK)
    {
        fprintf(stderr, "invalid command type %d from connection %u\n", cmd_ack.type, conn_id);
        exit(EXIT_FAILURE);
    }

    // printf("Node %u: received ack from Node %u\n", self_conn_id, conn_id);

    printf("Node:: successfully connected to conn_id: %u\n", conn_id);
}
