#include "model/ConvertibleCode.hh"
#include "model/ClusterSettings.hh"
#include "util/StripeGenerator.hh"
#include "model/StripeBatch.hh"
#include "util/Utils.hh"

int main(int argc, char *argv[])
{

    if (argc != 8)
    {
        printf("usage: ./GenPrePlacement k_i m_i k_f m_f num_nodes num_stripes pre_placement_filename");
        return -1;
    }

    uint8_t k_i = atoi(argv[1]);
    uint8_t m_i = atoi(argv[2]);
    uint8_t k_f = atoi(argv[3]);
    uint8_t m_f = atoi(argv[4]);
    uint16_t num_nodes = atoi(argv[5]);
    uint32_t num_stripes = atoi(argv[6]);
    string pre_placement_filename = argv[7];

    StripeGenerator stripe_generator;

    // initialize code
    ConvertibleCode code(k_i, m_i, k_f, m_f);
    ClusterSettings settings(num_nodes, num_stripes);
    code.print();
    settings.print();

    // check the number of stripes are valid
    if (code.isValidForPM() == false || settings.isParamValid(code) == false)
    {
        printf("invalid parameters\n");
        return -1;
    }

    // random generator
    mt19937 random_generator = Utils::createRandomGenerator();

    // generate random stripes, and store in the placement file
    StripeBatch stripe_batch(0, code, settings, random_generator);
    stripe_generator.genRandomStripes(code, settings, random_generator, stripe_batch.pre_stripes);

    stripe_generator.storeStripes(stripe_batch.pre_stripes, pre_placement_filename);

    return 0;
}