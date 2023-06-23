#include "Node.hh"

Node::Node(uint16_t _self_conn_id, Config &_config) : self_conn_id(_self_conn_id), config(_config)
{
    // init socket
    sockpp::socket_initializer::initialize();

    if (self_conn_id != CTRL_NODE_ID)
    {
        // add controller
        addrs_map[CTRL_NODE_ID] = pair<string, unsigned int>(config.controller_addr.first, config.controller_addr.second);
        connectors_map[CTRL_NODE_ID] = sockpp::tcp_connector();
        sockets_map[CTRL_NODE_ID] = sockpp::tcp_socket();
    }

    // add Agent
    for (auto &item : config.agent_addr_map)
    {
        uint16_t conn_id = item.first;
        auto &agent_addr = item.second;
        if (self_conn_id != conn_id)
        {
            addrs_map[conn_id] = pair<string, unsigned int>(agent_addr.first, agent_addr.second);
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
    thread ack_conn_thread(Node::ackConnAllSockets, self_conn_id, &sockets_map, acceptor);

    // connect all nodes
    Node::connectAllSockets(self_conn_id, &connectors_map, &addrs_map);

    // join ack connector threads
    ack_conn_thread.join();
}

Node::~Node()
{
    acceptor->close();
    delete acceptor;
}

void Node::connectAllSockets(uint16_t self_conn_id, unordered_map<uint16_t, sockpp::tcp_connector> *connectors_map, unordered_map<uint16_t, pair<string, unsigned int>> *addrs_map)
{
    unordered_map<uint16_t, thread *> conn_threads;

    // create connect threads
    for (auto &item : *connectors_map)
    {
        uint16_t conn_id = item.first;
        string ip = addrs_map->at(conn_id).first;
        unsigned int port = addrs_map->at(conn_id).second;
        conn_threads[conn_id] = new thread(Node::connectOneSocket, self_conn_id, connectors_map, conn_id, ip, port);
    }

    for (auto &item : conn_threads)
    {
        item.second->join();
        delete item.second;
    }

    // create handle ack threads
    unordered_map<uint16_t, thread *> ack_threads;
    for (auto &item : *connectors_map)
    {
        uint16_t conn_id = item.first;
        ack_threads[conn_id] = new thread(Node::handleAckOneSocket, self_conn_id, connectors_map, conn_id);
    }

    for (auto &item : ack_threads)
    {
        item.second->join();
        delete item.second;
    }

    printf("Node::connectAllSockets successfully connected to %lu nodes\n", connectors_map->size());
}

void Node::connectOneSocket(uint16_t self_conn_id, unordered_map<uint16_t, sockpp::tcp_connector> *connectors_map, uint16_t conn_id, string ip, uint16_t port)
{
    // create connection
    sockpp::tcp_connector &connector = connectors_map->at(conn_id);
    while (!(connector = sockpp::tcp_connector(sockpp::inet_address(ip, port))))
    {
        this_thread::sleep_for(chrono::milliseconds(1));
    }

    // printf("Successfully created connection to conn_id: %u (%s, %u)\n", conn_id, ip.c_str(), port);

    // send the connection command
    Command cmd_conn;
    cmd_conn.buildCommand(CommandType::CMD_CONN, self_conn_id, conn_id);

    if (connector.write_n(cmd_conn.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
    {
        fprintf(stderr, "Node::connectOneSocket error send cmd_conn\n");
        exit(EXIT_FAILURE);
    }

    // printf("Node::connectOneSocket send connection command to %u \n", conn_id);
    // Utils::printUCharBuffer(cmd_conn.content, MAX_CMD_LEN);
}

void Node::handleAckOneSocket(uint16_t self_conn_id, unordered_map<uint16_t, sockpp::tcp_connector> *connectors_map, uint16_t conn_id)
{
    sockpp::tcp_connector &connector = connectors_map->at(conn_id);

    // parse the ack command
    Command cmd_ack;
    if (connector.read_n(cmd_ack.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
    {
        fprintf(stderr, "Node::handleAckOneSocket error reading cmd_ack from %u\n", conn_id);
        exit(EXIT_FAILURE);
    }
    cmd_ack.parse();

    // printf("Node::handleAckOneSocket received command from %u \n", conn_id);
    // Utils::printUCharBuffer(cmd_ack.content, MAX_CMD_LEN);

    if (cmd_ack.type != CommandType::CMD_ACK)
    {
        fprintf(stderr, "Node::handleAckOneSocket invalid command type %d from connection %u\n", cmd_ack.type, conn_id);
        exit(EXIT_FAILURE);
    }

    printf("Node::handleAckOneSocket successfully build connection: (%u <-> %u)\n", self_conn_id, conn_id);
}

void Node::ackConnAllSockets(uint16_t self_conn_id, unordered_map<uint16_t, sockpp::tcp_socket> *sockets_map, sockpp::tcp_acceptor *acceptor)
{
    uint16_t num_acked_nodes = 0;
    uint16_t num_conns = sockets_map->size();
    while (num_acked_nodes < num_conns)
    {
        sockpp::inet_address conn_addr;
        sockpp::tcp_socket skt = acceptor->accept(&conn_addr);

        if (!skt)
        {
            fprintf(stderr, "Node::ackConnAllSockets invalid socket: %s\n", acceptor->last_error_str().c_str());
            exit(EXIT_FAILURE);
        }

        // printf("Node::ackConnAllSockets received from %s\n", conn_addr.to_string().c_str());

        // parse the connection command
        Command cmd_conn;
        if (skt.read_n(cmd_conn.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
        {
            fprintf(stderr, "Node::ackConnAllSockets error reading cmd_conn\n");
            exit(EXIT_FAILURE);
        }

        cmd_conn.parse();

        if (cmd_conn.type != CommandType::CMD_CONN || cmd_conn.dst_conn_id != self_conn_id)
        {
            fprintf(stderr, "Node::ackConnAllSockets invalid cmd_conn: type: %u, dst_conn_id: %u\n", cmd_conn.type, cmd_conn.dst_conn_id);
            exit(EXIT_FAILURE);
        }

        // printf("Node::ackConnAllSockets received connection command\n");
        // Utils::printUCharBuffer(cmd_conn.content, MAX_CMD_LEN);

        // maintain the socket
        uint16_t conn_id = cmd_conn.src_conn_id;
        sockets_map->at(conn_id) = move(skt);

        // send the ack command
        auto &reply_skt = sockets_map->at(conn_id);

        Command cmd_ack;
        cmd_ack.buildCommand(CommandType::CMD_ACK, self_conn_id, conn_id);

        if (reply_skt.write_n(cmd_ack.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
        {
            fprintf(stderr, "Node::ackConnAllSockets error send cmd_ack\n");
            exit(EXIT_FAILURE);
        }

        num_acked_nodes++;

        // printf("Node::ackConnAllSockets received connection and acked to %u; connected to (%u / %u) Nodes\n", conn_id, num_acked_nodes, num_conns);
        // Utils::printUCharBuffer(cmd_ack.content, MAX_CMD_LEN);
    }
}