#ifndef __COMMAND_HH__
#define __COMMAND_HH__

#include "../include/include.hh"
#include <arpa/inet.h>

enum CommandType
{
    CMD_CONN, // create connection: type | src_conn_id | dst_conn_id
    CMD_ACK,  // acknowledge: type | src_conn_id | dst_conn_id
    CMD_STOP, // stop connection: type | src_conn_id | dst_conn_id
    CMD_READ_BLK,
    CMD_WRITE_BLK,
    CMD_DELETE_BLK,
    CMD_TRANSFER_BLK,
    CMD_COMPUTE_BLK,
    AGCMD_SEND_BLK,
    AGCMD_ACK,
    CMD_UNKNOWN
};

class Command
{
private:
    /* data */
public:
    CommandType type;
    uint64_t len; // command length
    unsigned char content[MAX_CMD_LEN];
    uint16_t src_conn_id; // source connection id
    uint16_t dst_conn_id; // dst connection id

    Command();
    ~Command();

    // type parsing
    void writeUInt(unsigned int val);
    unsigned int readUInt();
    void writeUInt16(uint16_t val);
    uint16_t readUInt16();

    // command types
    void parse();

    // connection
    void buildConn(uint16_t _src_conn_id, uint16_t _dst_conn_id);

    // ack
    void buildAck(uint16_t _src_conn_id, uint16_t _dst_conn_id);

    // stop
    void buildStop(uint16_t _src_conn_id, uint16_t _dst_conn_id);
};

#endif // __COMMAND_HH__