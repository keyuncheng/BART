#include "Command.hh"

Command::Command(/* args */)
{
    type = CommandType::CMD_UNKNOWN;
    len = 0; // command length
    memset(content, 0, MAX_CMD_LEN);
}

Command::~Command()
{
}

void Command::print()
{
    printf("Command %u, conn: (%u -> %u)", type, src_conn_id, dst_conn_id);
    switch (type)
    {
    case CommandType::CMD_CONN:
    case CommandType::CMD_ACK:
    case CommandType::CMD_STOP:
    case CommandType::CMD_UNKNOWN:
    {
        printf("\n");
        break;
    }
    case CommandType::CMD_COMPUTE_RE_BLK:
    case CommandType::CMD_COMPUTE_PM_BLK:
    case CommandType::CMD_READ_COMPUTE_BLK:
    case CommandType::CMD_TRANSFER_COMPUTE_BLK:
    case CommandType::CMD_TRANSFER_RELOC_BLK:
    case CommandType::CMD_DELETE_BLK:
    {
        printf(", post_stripe: (%u, %u), transfer(%u, %u), enc_method: %u, num_src_blocks: %u, num_parity_reloc_blocks: %u\n", post_stripe_id, post_block_id, src_node_id, dst_node_id, enc_method, num_src_blocks, num_parity_reloc_blocks);
        printf("src_block_path: %s, dst_block_path: %s\n", src_block_path.c_str(), dst_block_path.c_str());

        if (type == CommandType::CMD_COMPUTE_RE_BLK || type == CommandType::CMD_COMPUTE_PM_BLK)
        {
            printf("src_block_nodes:\n");
            Utils::printVector(src_block_nodes);
            printf("parity_reloc_nodes:\n");
            Utils::printVector(parity_reloc_nodes);
        }

        break;
    }
    }
}

void Command::writeUInt(unsigned int val)
{
    unsigned int ns_val = htonl(val);
    memcpy(content + len, (unsigned char *)&ns_val, sizeof(unsigned int));
    len += sizeof(unsigned int);
}

unsigned int Command::readUInt()
{
    unsigned int val;
    memcpy((unsigned char *)&val, content + len, sizeof(unsigned int));
    len += sizeof(unsigned int);
    return ntohl(val);
}

void Command::writeUInt16(uint16_t val)
{
    uint16_t ns_val = htons(val);
    memcpy(content + len, (unsigned char *)&ns_val, sizeof(uint16_t));
    len += sizeof(uint16_t);
}

uint16_t Command::readUInt16()
{
    uint16_t val;
    memcpy((unsigned char *)&val, content + len, sizeof(uint16_t));
    len += sizeof(uint16_t);
    return ntohs(val);
}

void Command::writeString(string &val)
{
    uint32_t slen = val.length();
    writeUInt(slen);
    // string
    if (slen > 0)
    {
        memcpy(content + len, val.c_str(), slen);
        len += slen;
    }
}

string Command::readString()
{
    string val;
    unsigned int slen = readUInt();
    if (slen > 0)
    {
        char *raw_str = (char *)calloc(sizeof(char), slen + 1);
        memcpy(raw_str, content + len, slen);
        len += slen;
        val = string(raw_str);
        free(raw_str);
    }
    return val;
}

void Command::parse()
{
    // the command string has been stored in content

    type = (CommandType)readUInt(); // type
    src_conn_id = readUInt16();     // src conn id
    dst_conn_id = readUInt16();     // dst conn id
    switch (type)
    {
    case CommandType::CMD_CONN:
    case CommandType::CMD_ACK:
    {
        break;
    }
    case CommandType::CMD_COMPUTE_RE_BLK:
    case CommandType::CMD_COMPUTE_PM_BLK:
    case CommandType::CMD_STOP:
    case CommandType::CMD_READ_COMPUTE_BLK:
    case CommandType::CMD_TRANSFER_COMPUTE_BLK:
    case CommandType::CMD_TRANSFER_RELOC_BLK:
    case CommandType::CMD_DELETE_BLK:
    {
        post_stripe_id = readUInt();
        post_block_id = readUInt16();
        src_node_id = readUInt16();
        dst_node_id = readUInt16();
        src_block_path = readString();
        dst_block_path = readString();

        // additional information for parity compute and relocation
        if (type == CommandType::CMD_COMPUTE_RE_BLK || type == CommandType::CMD_COMPUTE_PM_BLK)
        {

            enc_method = (EncodeMethod)readUInt();
            num_src_blocks = readUInt();
            src_block_nodes.resize(num_src_blocks);
            for (uint8_t idx = 0; idx < num_src_blocks; idx++)
            {
                src_block_nodes[idx] = readUInt16();
            }
            num_parity_reloc_blocks = readUInt();
            parity_reloc_nodes.resize(num_parity_reloc_blocks);
            for (uint8_t idx = 0; idx < num_parity_reloc_blocks; idx++)
            {
                parity_reloc_nodes[idx] = readUInt16();
            }
        }

        if (type == CommandType::CMD_TRANSFER_RELOC_BLK)
        {
        }
        break;
    }
    case CommandType::CMD_UNKNOWN:
    {
        fprintf(stderr, "invalid command type\n");
        exit(EXIT_FAILURE);
    }
    }
}

void Command::buildCommand(CommandType _type, uint16_t _src_conn_id, uint16_t _dst_conn_id)
{
    type = _type;
    src_conn_id = _src_conn_id;
    dst_conn_id = _dst_conn_id;

    writeUInt(type);          // type
    writeUInt16(src_conn_id); // src conn id
    writeUInt16(dst_conn_id); // dst conn id
}

void Command::buildCommand(CommandType _type, uint16_t _src_conn_id, uint16_t _dst_conn_id, uint32_t _post_stripe_id, uint8_t _post_block_id, uint16_t _src_node_id, uint16_t _dst_node_id, string _src_block_path, string _dst_block_path)
{
    buildCommand(_type, _src_conn_id, _dst_conn_id);

    post_stripe_id = _post_stripe_id;
    post_block_id = _post_block_id;
    src_node_id = _src_node_id;
    dst_node_id = _dst_node_id;
    src_block_path = _src_block_path;
    dst_block_path = _dst_block_path;

    writeUInt(post_stripe_id);
    writeUInt16(post_block_id);
    writeUInt16(src_node_id);
    writeUInt16(dst_node_id);
    writeString(src_block_path);
    writeString(dst_block_path);
}

// CMD_TRANSFER_BLK (newly added)
void Command::buildCommand(CommandType _type, uint16_t _src_conn_id, uint16_t _dst_conn_id, string _dst_block_path)
{
    buildCommand(_type, _src_conn_id, _dst_conn_id);

    writeString(dst_block_path);
}

// CMD_COMPUTE_RE_BLK, CMD_COMPUTE_PM_BLK
void Command::buildCommand(CommandType _type, uint16_t _src_conn_id, uint16_t _dst_conn_id, uint32_t _post_stripe_id, uint8_t _post_block_id, uint16_t _src_node_id, uint16_t _dst_node_id, string _src_block_path, string _dst_block_path, EncodeMethod _enc_method, uint8_t _num_src_blocks, vector<uint16_t> _src_block_nodes, uint8_t _num_parity_reloc_blocks, vector<uint16_t> _parity_reloc_nodes)
{
    buildCommand(_type, _src_conn_id, _dst_conn_id, _post_stripe_id, _post_block_id, _src_node_id, _dst_node_id, _src_block_path, _dst_block_path);

    // additional information for parity compute and relocation
    enc_method = _enc_method;
    num_src_blocks = _num_src_blocks;
    src_block_nodes = _src_block_nodes;
    num_parity_reloc_blocks = _num_parity_reloc_blocks;
    parity_reloc_nodes = _parity_reloc_nodes;

    writeUInt(enc_method);
    writeUInt(num_src_blocks);
    for (uint8_t idx = 0; idx < num_src_blocks; idx++)
    {
        writeUInt16(src_block_nodes[idx]);
    }
    writeUInt(num_parity_reloc_blocks);
    for (uint8_t idx = 0; idx < num_parity_reloc_blocks; idx++)
    {
        writeUInt16(parity_reloc_nodes[idx]);
    }
}