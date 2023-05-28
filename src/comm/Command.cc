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
    case CommandType::CMD_TRANSFER_COMPUTE_BLK:
    case CommandType::CMD_TRANSFER_RELOC_BLK:
    case CommandType::AGCMD_SEND_BLK:
    case CommandType::AGCMD_ACK:
    {
        printf(", sg_id: %u, stripe_id_global: %u, stripe_id: %u, block_id: %u, block_src_id: %u, block_dst_id: %u, block_path: %s\n", block_path.c_str());
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

void Command::writeString(string &s)
{
}

string Command::readString()
{
    return "";
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
    case CommandType::CMD_TRANSFER_COMPUTE_BLK:
    case CommandType::CMD_TRANSFER_RELOC_BLK:
    case CommandType::AGCMD_SEND_BLK:
    case CommandType::AGCMD_ACK:
    {
        sg_id = readUInt();
        stripe_id_global = readUInt();
        stripe_id = readUInt16();
        block_id = readUInt16();
        block_src_id = readUInt16();
        block_dst_id = readUInt16();
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

void Command::buildCommand(CommandType _type, uint16_t _src_conn_id, uint16_t _dst_conn_id, uint32_t _sg_id, uint32_t _stripe_id_global, uint8_t _stripe_id, uint8_t _block_id, uint16_t _block_src_id, uint16_t _block_dst_id, string _block_path)
{
    buildCommand(_type, _src_conn_id, _dst_conn_id);

    sg_id = _sg_id;
    stripe_id_global = _stripe_id_global;
    stripe_id = _stripe_id;
    block_id = _block_id;
    block_src_id = _block_src_id;
    block_dst_id = _block_dst_id;
    block_path = _block_path;

    writeUInt(sg_id);
    writeUInt(stripe_id_global);
    writeUInt16(stripe_id);
    writeUInt16(block_id);
    writeUInt16(block_src_id);
    writeUInt16(block_dst_id);
    writeString(block_path);
}