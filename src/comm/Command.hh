#ifndef __COMMAND_HH__
#define __COMMAND_HH__

#include "../include/include.hh"
#include <arpa/inet.h>

enum CommandType
{
    /**
     * @brief connection; acknowledge; stop connection;
     * format: <type | src_conn_id | dst_conn_id>
     */
    CMD_CONN,
    CMD_ACK,
    CMD_STOP,
    /**
     * @brief block computation and transfer: computation; relocation (from Controller or from Agent);
     * format: <type | src_conn_id | dst_conn_id | stripe_group_id | stripe_id_global | stripe_id | block_id | block_src_id | block_dst_id | block_path>
     */
    CMD_COMPUTE_BLOCK,
    CMD_TRANSFER_COMPUTE_BLK,
    CMD_TRANSFER_RELOC_BLK,
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
    uint64_t len;                       // command length
    unsigned char content[MAX_CMD_LEN]; // content
    uint16_t src_conn_id;               // source connection id
    uint16_t dst_conn_id;               // dst connection id
    uint32_t sg_id;                     // stripe group id
    uint32_t stripe_id_global;          // global stripe id
    uint8_t stripe_id;                  // stripe id in group
    uint8_t block_id;                   // block id in group
    uint16_t block_src_id;              // block source node id
    uint16_t block_dst_id;              // block destination node id
    string block_path;                  // block physical path

    Command();
    ~Command();

    void print();

    // type parsing
    void writeUInt(unsigned int val);
    unsigned int readUInt();
    void writeUInt16(uint16_t val);
    uint16_t readUInt16();
    void writeString(string &s);
    string readString();

    // command types
    void parse();

    // CMD_CONN, CMD_ACK, CMD_STOP
    void buildCommand(CommandType _type, uint16_t _src_conn_id, uint16_t _dst_conn_id);

    // CMD_TRANSFER_COMPUTE_BLK, CMD_TRANSFER_RELOC_BLK, AGCMD_SEND_BLK, AGCMD_ACK
    void buildCommand(CommandType _type, uint16_t _src_conn_id, uint16_t _dst_conn_id, uint32_t _sg_id, uint32_t _stripe_id_global, uint8_t _stripe_id, uint8_t _block_id, uint16_t _block_src_id, uint16_t _block_dst_id, string _block_path);
};

#endif // __COMMAND_HH__