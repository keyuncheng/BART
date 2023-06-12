#include <isa-l.h>
#include "include/include.hh"
#include "comm/BlockIO.hh"

int main(int argc, char *argv[])
{
    if (argc != 7)
    {
        printf("usage: ./GenECStripe k_i m_i k_f m_f block_size data_dir");
        return -1;
    }

    uint8_t k_i = atoi(argv[1]);
    uint8_t m_i = atoi(argv[2]);
    uint8_t k_f = atoi(argv[3]);
    uint8_t m_f = atoi(argv[4]);
    char *end;
    uint64_t block_size = strtoull(argv[5], &end, 10);
    string data_dir = argv[6];

    uint8_t n_i = k_i + m_i;
    uint8_t n_f = k_f + m_f;

    uint8_t num_pre_stripes = k_f / k_i;

    if (k_f % k_i != 0 || m_f > m_i)
    {
        printf("invalid input parameters\n");
        exit(EXIT_FAILURE);
    }

    if (data_dir.find_last_of("/") == string::npos)
    {
        printf("invalid input data directory\n");
        exit(EXIT_FAILURE);
    }

    printf("transition: (%u, %u) -> (%u, %u), block size: %lu", k_i, m_i, k_f, m_f, block_size);

    // we only use m_f afterwards

    // init pre-transition encoding table
    unsigned char *pre_matrix = (unsigned char *)malloc(n_i * k_i * sizeof(unsigned char));
    unsigned char *pre_encode_gftbl = (unsigned char *)malloc(32 * k_i * m_f * sizeof(unsigned char));
    gf_gen_rs_matrix(pre_matrix, n_i, k_i); // Vandermonde matrix
    ec_init_tables(k_i, m_f, &pre_matrix[k_i * k_i], pre_encode_gftbl);

    // init post-transition encoding table
    unsigned char *post_matrix = (unsigned char *)malloc(n_f * k_f * sizeof(unsigned char));
    unsigned char *post_encode_gftbl = (unsigned char *)malloc(32 * k_f * m_f * sizeof(unsigned char));
    gf_gen_rs_matrix(post_matrix, n_f, k_f); // Vandermonde matrix
    ec_init_tables(k_f, m_f, &post_matrix[k_f * k_f], post_encode_gftbl);

    // buffers
    unsigned char **post_data_buffer = (unsigned char **)calloc(k_f, sizeof(unsigned char *));
    unsigned char **post_parity_buffer = (unsigned char **)calloc(m_f, sizeof(unsigned char *));

    for (uint8_t data_id = 0; data_id < k_f; data_id++)
    {
        uint8_t pre_block_id = data_id % k_i;
        post_data_buffer[data_id] = (unsigned char *)calloc(block_size, sizeof(unsigned char));
        memset(post_data_buffer[data_id], pre_block_id, block_size); // initialize the content
    }

    for (uint8_t parity_id = 0; parity_id < m_f; parity_id++)
    {
        post_parity_buffer[parity_id] = (unsigned char *)calloc(block_size, sizeof(unsigned char));
    }

    // store data blocks
    for (uint8_t pre_stripe_id = 0; pre_stripe_id < num_pre_stripes; pre_stripe_id++)
    {
        for (uint8_t pre_data_id = 0; pre_data_id < k_i; pre_data_id++)
        {
            string data_block_path = data_dir + string("block_") + to_string(pre_stripe_id) + string("_") + to_string(pre_data_id);

            BlockIO::writeBlock(data_block_path, post_data_buffer[pre_stripe_id * k_i + pre_data_id], block_size);
        }
    }

    // store parity blocks for pre-stripes
    for (uint8_t pre_stripe_id = 0; pre_stripe_id < num_pre_stripes; pre_stripe_id++)
    { // only encode for m_f parities
        ec_encode_data(block_size, k_i, m_f, pre_encode_gftbl, &post_data_buffer[pre_stripe_id * k_i], post_parity_buffer);

        for (uint8_t parity_id = 0; parity_id < m_f; parity_id++)
        {
            string pre_parity_block_path = data_dir + string("block_") + to_string(pre_stripe_id) + string("_") + to_string(k_i + parity_id);

            BlockIO::writeBlock(pre_parity_block_path, post_parity_buffer[parity_id], block_size);
        }
    }

    // store parity blocks for post-stripes
    ec_encode_data(block_size, k_f, m_f, post_encode_gftbl, post_data_buffer, post_parity_buffer);

    for (uint8_t parity_id = 0; parity_id < m_f; parity_id++)
    {
        string post_parity_block_path = data_dir + string("post_block_") + to_string(k_f + parity_id);

        BlockIO::writeBlock(post_parity_block_path, post_parity_buffer[parity_id], block_size);
    }

    // free buffer
    for (uint8_t data_id = 0; data_id < k_f; data_id++)
    {
        free(post_data_buffer[data_id]);
    }

    for (uint8_t parity_id = 0; parity_id < m_f; parity_id++)
    {
        free(post_parity_buffer[parity_id]);
    }
    free(post_data_buffer);
    free(post_parity_buffer);

    free(post_matrix);
    free(post_encode_gftbl);
    free(pre_matrix);
    free(pre_encode_gftbl);

    return 0;
}