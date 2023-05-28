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
    printf("Command type: %u, src_conn_id: %u, dst_conn_id: %u");
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
    case CommandType::CMD_READ_RE_BLK:
    case CommandType::CMD_TRANSFER_COMPUTE_RE_BLK:
    case CommandType::CMD_COMPUTE_RE_BLK:
    case CommandType::CMD_READ_PM_BLK:
    case CommandType::CMD_TRANSFER_COMPUTE_PM_BLK:
    case CommandType::CMD_COMPUTE_PM_BLK:
    case CommandType::CMD_READ_RELOC_BLK:
    case CommandType::CMD_TRANSFER_RELOC_BLK:
    case CommandType::CMD_WRITE_BLK:
    case CommandType::CMD_DELETE_BLK:
    {
        post_stripe_id = readUInt();
        post_block_id = readUInt16();
        pre_stripe_id_global = readUInt();
        pre_stripe_id_relative = readUInt16();
        pre_block_id = readUInt16();
        src_node_id = readUInt16();
        dst_node_id = readUInt16();
        block_path = readString();

        printf(", post_stripe: (%u, %u), pre_stripe: (%u, %u, %u), src_node: %u, dst_node: %u, block_path: %s\n", post_stripe_id, post_block_id, pre_stripe_id_global, pre_stripe_id_relative, pre_block_id, src_node_id, dst_node_id, block_path.c_str());
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
    case CommandType::CMD_READ_RE_BLK:
    case CommandType::CMD_TRANSFER_COMPUTE_RE_BLK:
    case CommandType::CMD_COMPUTE_RE_BLK:
    case CommandType::CMD_READ_PM_BLK:
    case CommandType::CMD_TRANSFER_COMPUTE_PM_BLK:
    case CommandType::CMD_COMPUTE_PM_BLK:
    case CommandType::CMD_READ_RELOC_BLK:
    case CommandType::CMD_TRANSFER_RELOC_BLK:
    case CommandType::CMD_WRITE_BLK:
    case CommandType::CMD_DELETE_BLK:
    {
        post_stripe_id = readUInt();
        post_block_id = readUInt16();
        pre_stripe_id_global = readUInt();
        pre_stripe_id_relative = readUInt16();
        pre_block_id = readUInt16();
        src_node_id = readUInt16();
        dst_node_id = readUInt16();
        block_path = readString();
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

void Command::buildCommand(CommandType _type, uint16_t _src_conn_id, uint16_t _dst_conn_id, uint32_t _post_stripe_id, uint8_t _post_block_id, uint32_t _pre_stripe_id_global, uint8_t _pre_stripe_id_relative, uint8_t _pre_block_id, uint16_t _src_node_id, uint16_t _dst_node_id, string _block_path)
{
    buildCommand(_type, _src_conn_id, _dst_conn_id);

    post_stripe_id = _post_stripe_id;
    post_block_id = _post_block_id;
    pre_stripe_id_global = _pre_stripe_id_global;
    pre_stripe_id_relative = _pre_stripe_id_relative;
    pre_block_id = _pre_block_id;
    src_node_id = _src_node_id;
    dst_node_id = _dst_node_id;
    block_path = _block_path;

    writeUInt(post_stripe_id);
    writeUInt16(post_block_id);
    writeUInt(pre_stripe_id_global);
    writeUInt16(pre_stripe_id_relative);
    writeUInt16(pre_block_id);
    writeUInt16(src_node_id);
    writeUInt16(dst_node_id);
    writeString(block_path);
}