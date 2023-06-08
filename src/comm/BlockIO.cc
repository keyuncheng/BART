#include "BlockIO.hh"

BlockIO::BlockIO(/* args */)
{
}

BlockIO::~BlockIO()
{
}

uint64_t BlockIO::readBlock(string block_path, unsigned char *buffer, uint64_t block_size)
{
    FILE *file = fopen(block_path.c_str(), "r");
    if (!file)
    {
        fprintf(stderr, "failed to open file %s\n", block_path.c_str());
        return false;
    }

    uint64_t read_bytes = 0;
    uint64_t offset = 0;
    uint64_t bytes_left = block_size;
    while (!feof(file) && (read_bytes = fread(buffer + offset, 1, bytes_left, file)) > 0)
    {
        bytes_left -= read_bytes;
        offset += read_bytes;
    }
    fclose(file);

    return offset;
}

uint64_t BlockIO::writeBlock(string block_path, unsigned char *buffer, uint64_t block_size)
{
    // write file
    FILE *file = fopen(block_path.c_str(), "w");
    if (!file)
    {
        fprintf(stderr, "failed to open file %s\n", block_path.c_str());
        return false;
    }
    uint64_t write_bytes = 0;
    uint64_t offset = 0;
    uint64_t bytes_left = block_size;
    while (offset < block_size && (write_bytes = fwrite(buffer + offset, 1, bytes_left, file)))
    {
        bytes_left -= write_bytes;
        offset += write_bytes;
    }
    fclose(file);

    return offset;
}

void BlockIO::deleteBlock(string block_path)
{
    // remove file
    // std::remove(block_path.c_str());
    printf("remove block %s\n", block_path.c_str());
}

uint64_t BlockIO::sendBlock(sockpp::tcp_connector &connector, unsigned char *buffer, uint64_t block_size)
{
    uint64_t send_bytes = 0;
    uint64_t offset = 0;
    uint64_t bytes_left = block_size;
    while (offset < block_size && (send_bytes = connector.write_n(buffer + offset, bytes_left * sizeof(unsigned char))))
    {
        bytes_left -= send_bytes;
        offset += send_bytes;
    }

    return offset;
}

uint64_t BlockIO::recvBlock(sockpp::tcp_socket &skt, unsigned char *buffer, uint64_t block_size)
{
    uint64_t recv_bytes = 0;
    uint64_t offset = 0;
    uint64_t bytes_left = block_size;
    while (offset < block_size && (recv_bytes = skt.read_n(buffer + offset, bytes_left * sizeof(unsigned char))))
    {
        bytes_left -= recv_bytes;
        offset += recv_bytes;
    }

    return offset;
}
