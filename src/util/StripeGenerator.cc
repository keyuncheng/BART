#include "StripeGenerator.hh"

StripeGenerator::StripeGenerator(/* args */)
{
}

StripeGenerator::~StripeGenerator()
{
}

vector<Stripe> StripeGenerator::GenerateStripes(ConvertibleCode code, ClusterSettings settings) {
    int num_stripes = settings.N;
    int num_nodes = settings.M;
    // random number generator
    const int range_from = 0;
    const int range_to = num_nodes - 1;
    std::random_device rand_dev;
    std::mt19937 generator(rand_dev());
    std::uniform_int_distribution<int> distr(range_from, range_to);

    vector<Stripe> stripes;

    for (int stripe_id = 0; stripe_id < num_stripes; stripe_id++) {
        vector<int> stripe_indices;
        
        // generate indices of stripe
        while (stripe_indices.size() < (size_t) code.n_i) {
            int random_node = distr(generator);
            auto it = find(stripe_indices.begin(), stripe_indices.end(), random_node);

            if (it == stripe_indices.end()) {
                stripe_indices.push_back(random_node);
            }
        }

        Stripe stripe(code, settings, stripe_id, stripe_indices);
        stripes.push_back(stripe);
    }

    return stripes;
}