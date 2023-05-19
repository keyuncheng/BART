#include "Node.hh"

Node::Node(uint16_t _self_conn_id, Config &_config) : self_conn_id(_self_conn_id), config(_config)
{
    // init socket
    sockpp::socket_initializer sock_init;

    // add Controller
    if (self_conn_id != CTRL_NODE_ID)
    {
        connectors_map[CTRL_NODE_ID] = sockpp::tcp_connector();
        sockets_map[CTRL_NODE_ID] = sockpp::tcp_socket();
    }

    // add Agent
    for (auto &item : config.agent_ip_map)
    {
        uint16_t conn_id = item.first;
        if (self_conn_id != conn_id)
        {
            connectors_map[conn_id] = sockpp::tcp_connector();
            sockets_map[conn_id] = sockpp::tcp_socket();
        }
    }

    // add acceptor
    acceptor = new sockpp::tcp_acceptor(config.port);

    printf("Node %u: start connection\n", self_conn_id);

    // create ack connector threads
    thread ack_conn_thread([&]
                           { Node::ack_conn_all(); });

    // connect all nodes
    connect_all();

    // join ack connector threads
    ack_conn_thread.join();
}

Node::~Node()
{

    delete acceptor;
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

    // create handle ack threads
    unordered_map<uint16_t, thread *> ack_threads;
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;
        ack_threads[conn_id] = new thread(&Node::handle_ack_one, this, conn_id);
    }

    for (auto &item : ack_threads)
    {
        item.second->join();
        delete item.second;
    }

    printf("Node %u: successfully connected to %lu nodes\n", self_conn_id, connectors_map.size());
}

void Node::connect_one(uint16_t conn_id, string ip, uint16_t port)
{
    // create connection
    sockpp::tcp_connector &connector = connectors_map[conn_id];
    while (!(connector = sockpp::tcp_connector(sockpp::inet_address(ip, port))))
    {
        this_thread::sleep_for(chrono::milliseconds(1));
    }

    // send the connection command
    Command cmd_conn;
    cmd_conn.buildConn(self_conn_id, conn_id);

    if (connector.write_n(cmd_conn.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
    {
        fprintf(stderr, "error send cmd_conn\n");
        exit(EXIT_FAILURE);
    }
}

void Node::ack_conn_all()
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
        if (skt.read_n(&cmd_conn, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
        {
            fprintf(stderr, "error reading cmd_conn\n");
            exit(EXIT_FAILURE);
        }
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
        auto &connector = connectors_map[conn_id];
        Command cmd_ack;
        cmd_ack.buildAck(self_conn_id, conn_id);
        if (connector.write_n(cmd_ack.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
        {
            fprintf(stderr, "error send cmd_ack\n");
            exit(EXIT_FAILURE);
        }

        num_acked_nodes++;

        printf("Node %u: received connection and acked to Node %u; connected to (%u / %u) Nodes\n", self_conn_id, conn_id, num_acked_nodes, num_conns);
    }
}

void Node::handle_ack_one(uint16_t conn_id)
{
    sockpp::tcp_connector &connector = connectors_map[conn_id];

    // parse the ack command
    Command cmd_ack;
    if (connector.read_n(&cmd_ack, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
    {
        fprintf(stderr, "error reading cmd_ack from %u\n", conn_id);
        exit(EXIT_FAILURE);
    }
    cmd_ack.parse();
    if (cmd_ack.type != CommandType::CMD_ACK)
    {
        fprintf(stderr, "invalid command type %d from connection %u\n", cmd_ack.type, conn_id);
        exit(EXIT_FAILURE);
    }

    printf("Node %u: received ack from Node %u\n", self_conn_id, conn_id);
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
