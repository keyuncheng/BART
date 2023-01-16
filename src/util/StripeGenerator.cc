#include "StripeGenerator.hh"

StripeGenerator::StripeGenerator(/* args */)
{
}

StripeGenerator::~StripeGenerator()
{
}

vector<Stripe> StripeGenerator::GenerateStripes(ConvertibleCode code, ClusterSettings settings) {
    // random number generator
    const int range_from  = 0;
    const int range_to    = settings.M - 1;
    std::random_device rand_dev;
    std::mt19937 generator(rand_dev());
    std::uniform_int_distribution<int> distr(range_from, range_to);

    vector<Stripe> stripes;

    for (int i = 0; i < settings.N; i++) {
        vector<int> stripe_indices;
        
        // generate indices of stripe
        while (stripe_indices.size() < (size_t) code.n_i) {
            int random_node = distr(generator);
            auto it = find(stripe_indices.begin(), stripe_indices.end(), random_node);

            if (it == stripe_indices.end()) {
                stripe_indices.push_back(random_node);
            }
        }

        Stripe stripe(code, settings, i, stripe_indices);
        stripes.push_back(stripe);
    }

    return stripes;
}