#include "StripeGenerator.hh"

StripeGenerator::StripeGenerator(/* args */)
{
}

StripeGenerator::~StripeGenerator()
{
}

vector<Stripe> StripeGenerator::generateStripes(ConvertibleCode &code, ClusterSettings &settings) {
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

void StripeGenerator::storeStripes(vector<Stripe> &stripes, string placement_file) {
    if (stripes.size() == 0) {
        printf("invalid parameters\n");
        return;
    }

    ofstream of;

    of.open(placement_file, ios::out);

    for (auto stripe : stripes) {
        vector<int> &stripe_indices = stripe.getStripeIndices();

        for (auto idx : stripe_indices) {
            of << idx << " ";
        }

        of << endl;
    }

    of.close();
}

bool StripeGenerator::loadStripes(ConvertibleCode &code, ClusterSettings &settings, vector<Stripe> &stripes, string placement_file) {

    ifstream ifs(placement_file.c_str());

    if (ifs.fail()) {
        printf("invalid parameters\n");
        return false;
    }

    stripes.clear();

    string line;
    int stripe_id = 0;
    while (getline(ifs, line)) {
        istringstream iss(line);
        vector<int> stripe_indices;
        for (int block_id = 0; block_id < code.n_i; block_id++) {
            int idx;
            iss >> idx;
            stripe_indices.push_back(idx);
        }
        Stripe stripe(code, settings, stripe_id, stripe_indices);
        stripes.push_back(stripe);
        stripe_id++;
    }

    ifs.close();

    return true;
}