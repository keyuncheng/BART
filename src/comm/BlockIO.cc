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
        exit(EXIT_FAILURE);
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
    // mkdir
    string block_dir;
    auto it = block_path.find_last_of("/");
    if (it == string::npos)
    { // relative path; get current working directory
        char cwd[256];
        if (getcwd(cwd, 256))
        {
            block_dir = cwd;
        }
    }
    else
    { // absolute path
        block_dir = block_path.substr(0, it);
    }
    char mkdir_command[256];
    sprintf(mkdir_command, "mkdir -p %s", block_dir.c_str());
    if (system(mkdir_command) < 0)
    {
        fprintf(stderr, "BlockIO::writeBlock error create block directory: %s\n", block_dir.c_str());
    }
    // printf("BlockIO::writeBlock create block directory: %s\n", block_dir.c_str());

    // write file
    FILE *file = fopen(block_path.c_str(), "w");
    if (!file)
    {
        fprintf(stderr, "failed to open file %s\n", block_path.c_str());
        exit(EXIT_FAILURE);
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
