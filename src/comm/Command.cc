#include "Command.hh"

Command::Command(/* args */)
{
    type = CommandType::CMD_UNKNOWN;
    len = 0; // command length
}

Command::~Command()
{
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
    }
}

void Command::buildConn(uint16_t _src_conn_id, uint16_t _dst_conn_id)
{
    type = CommandType::CMD_CONN;
    src_conn_id = _src_conn_id;
    dst_conn_id = _dst_conn_id;

    writeUInt(type);          // type
    writeUInt16(src_conn_id); // src conn id
    writeUInt16(dst_conn_id); // dst conn id
}
void Command::buildAck(uint16_t _src_conn_id, uint16_t _dst_conn_id)
{
    type = CommandType::CMD_ACK;
    src_conn_id = _src_conn_id;
    dst_conn_id = _dst_conn_id;

    writeUInt(type);          // type
    writeUInt16(src_conn_id); // src conn id
    writeUInt16(dst_conn_id); // dst conn id
}

void Command::buildStop(uint16_t _src_conn_id, uint16_t _dst_conn_id)
{
    type = CommandType::CMD_STOP;
    src_conn_id = _src_conn_id;
    dst_conn_id = _dst_conn_id;

    writeUInt(type);          // type
    writeUInt16(src_conn_id); // src conn id
    writeUInt16(dst_conn_id); // dst conn id
}