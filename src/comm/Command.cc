#include "Command.hh"

Command::Command(/* args */)
{
    type = CommandType::CMD_UNKNOWN;
    len = 0; // command length
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
    case CommandType::CMD_LOCAL_COMPUTE_BLK:
    case CommandType::CMD_TRANSFER_COMPUTE_BLK:
    case CommandType::CMD_TRANSFER_RELOC_BLK:
    case CommandType::CMD_DELETE_BLK:
    {
        printf(", post_stripe: (%u, %u), transfer(%u, %u)\n", post_stripe_id, post_block_id, src_node_id, dst_node_id);
        // printf(", post_stripe: (%u, %u), transfer(%u, %u), src_block_path: %s, dst_block_path: %s\n", post_stripe_id, post_block_id, src_node_id, dst_node_id, src_block_path.c_str(), dst_block_path.c_str());
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
    case CommandType::CMD_STOP:
    {
        break;
    }
    case CommandType::CMD_LOCAL_COMPUTE_BLK:
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
        break;
    }
    case CommandType::CMD_UNKNOWN:
    {
        fprintf(stderr, "invalid command type\n");
        break;
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