#include "StripeGenerator.hh"

StripeGenerator::StripeGenerator(/* args */)
{
}

StripeGenerator::~StripeGenerator()
{
}

vector<Stripe> StripeGenerator::GenerateStripes(int k, int m, int N, int M) {
    // random number generator
    const int range_from  = 0;
    const int range_to    = M - 1;
    std::random_device rand_dev;
    std::mt19937 generator(rand_dev());
    std::uniform_int_distribution<int> distr(range_from, range_to);

    vector<Stripe> stripes;

    for (int i = 0; i < N; i++) {
        vector<int> stripe_indices;
        
        // generate indices of stripe
        while (stripe_indices.size() < (size_t) (k + m)) {
            int random_node = distr(generator);
            auto it = find(stripe_indices.begin(), stripe_indices.end(), random_node);

            if (it == stripe_indices.end()) {
                stripe_indices.push_back(random_node);
            }
        }

        Stripe stripe(k, m, M, stripe_indices);
        stripes.push_back(stripe);
    }

    return stripes;
}