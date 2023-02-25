#include "model/ConvertibleCode.hh"
#include "model/ClusterSettings.hh"
#include "util/StripeGenerator.hh"
#include "util/Utils.hh"

int main(int argc, char *argv[]) {

    if (argc != 8) {
        printf("usage: ./Simulator k_i m_i k_f m_f N M placement_file");
        return -1;
    }

    size_t k_i = atoi(argv[1]);
    size_t m_i = atoi(argv[2]);
    size_t k_f = atoi(argv[3]);
    size_t m_f = atoi(argv[4]);
    size_t N = atoi(argv[5]);
    size_t M = atoi(argv[6]);
    string placement_file = argv[7];

    StripeGenerator stripe_generator;

    // initialize code
    ConvertibleCode code(k_i, m_i, k_f, m_f);
    ClusterSettings settings;
    settings.N = N;
    settings.M = M;

    // check the number of stripes are valid
    if (Utils::isParamValid(code, settings) == false) {
        printf("invalid parameters\n");
        return -1;
    }

    // random generator
    mt19937 random_generator = Utils::createRandomGenerator();


    // generate random stripes, and store in the placement file
    vector<Stripe> stripes = stripe_generator.generateRandomStripes(code, settings, random_generator);
    stripe_generator.storeStripes(stripes, placement_file);

    printf("finished generating %ld (%ld, %ld) stripes in %ld storage nodes", settings.N, code.k_i, code.m_i, settings.M);

    return 0;
}