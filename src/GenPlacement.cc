#include "model/ConvertibleCode.hh"
#include "model/ClusterSettings.hh"
#include "util/StripeGenerator.hh"
#include "util/Utils.hh"

int main(int argc, char *argv[])
{

    if (argc != 8)
    {
        printf("usage: ./Simulator k_i m_i k_f m_f num_nodes num_stripes placement_file");
        return -1;
    }

    uint8_t k_i = atoi(argv[1]);
    uint8_t m_i = atoi(argv[2]);
    uint8_t k_f = atoi(argv[3]);
    uint8_t m_f = atoi(argv[4]);
    uint16_t num_nodes = atoi(argv[5]);
    uint32_t num_stripes = atoi(argv[6]);
    string placement_file = argv[7];

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
    vector<Stripe> stripes;
    stripe_generator.generateRandomStripes(code, settings, random_generator, stripes);

    stripe_generator.storeStripes(stripes, placement_file);

    // printf("stripes:\n");
    // for (auto &stripe : stripes)
    // {
    //     stripe.print();
    // }

    printf("finished generating stripes\n");

    return 0;
}