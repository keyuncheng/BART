#ifndef __COMMAND_HH__
#define __COMMAND_HH__

#include <arpa/inet.h>
#include "../include/include.hh"
#include "../model/StripeGroup.hh"

enum CommandType
{
    /**
     * @brief connection; acknowledge; stop connection;
     * format: <type | src_conn_id | dst_conn_id>
     */
    CMD_CONN,
    CMD_ACK,
    /**
     * @brief block computation and transfer: computation; relocation (from Controller or from Agent);
     * format: <type | src_conn_id | dst_conn_id | post_stripe_id | post_block_id | pre_stripe_id_global | pre_stripe_id_relative | pre_block_id | src_node_id | dst_node_id | block_path>
     */
    CMD_STOP,
    CMD_COMPUTE_RE_BLK,       // re-encoding compute command
    CMD_COMPUTE_PM_BLK,       // parity merging compute command
    CMD_READ_COMPUTE_BLK,     // read -> compute -> write
    CMD_TRANSFER_COMPUTE_BLK, // read -> transfer -> compute -> write
    CMD_TRANSFER_RELOC_BLK,   // read -> transfer -> write
    CMD_DELETE_BLK,           // delete
    CMD_UNKNOWN,
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

    uint32_t post_stripe_id; // post-transition stripe id (or stripe group id)
    uint8_t post_block_id;   // post-transition stripe block id
    uint16_t src_node_id;    // source node id
    uint16_t dst_node_id;    // destination node id
    string src_block_path;   // source block physical path (in the source node)
    string dst_block_path;   // destination block physical path (in the destination node)

    EncodeMethod enc_method;          // encode method
    uint8_t num_src_blocks;           // number of source blocks (re-encoding: code.k_f; parity merging: code.m_f)
    vector<uint16_t> src_block_nodes; // source block nodes

    uint8_t num_parity_reloc_blocks;     // number of nodes for parity relocation
    vector<uint16_t> parity_reloc_nodes; // parity relocation nodes (for re-encoding, reloc_nodes stores code.m_f nodes; for parity merging, reloc_nodes store the corresponding parity node only)

    Command();
    ~Command();

    void print();

    // type parsing
    void writeUInt(unsigned int val);
    unsigned int readUInt();
    void writeUInt16(uint16_t val);
    uint16_t readUInt16();
    void writeString(string &val);
    string readString();

    // command types
    void parse();

    // CMD_CONN, CMD_ACK, CMD_STOP
    void buildCommand(CommandType _type, uint16_t _src_conn_id, uint16_t _dst_conn_id);

    // CMD_READ_COMPUTE_BLK, CMD_TRANSFER_COMPUTE_BLK, CMD_TRANSFER_RELOC_BLK, CMD_DELETE_BLK
    void buildCommand(CommandType _type, uint16_t _src_conn_id, uint16_t _dst_conn_id, uint32_t _post_stripe_id, uint8_t _post_block_id, uint16_t _src_node_id, uint16_t _dst_node_id, string _src_block_path, string _dst_block_path);

    // CMD_COMPUTE_RE_BLK, CMD_COMPUTE_PM_BLK
    void buildCommand(CommandType _type, uint16_t _src_conn_id, uint16_t _dst_conn_id, uint32_t _post_stripe_id, uint8_t _post_block_id, uint16_t _src_node_id, uint16_t _dst_node_id, string _src_block_path, string _dst_block_path, EncodeMethod _enc_method, uint8_t _num_src_blocks, vector<uint16_t> _src_block_nodes, uint8_t _num_parity_reloc_blocks, vector<uint16_t> _parity_reloc_nodes);
};

#endif // __COMMAND_HH__