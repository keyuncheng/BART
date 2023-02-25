#include "StripeGenerator.hh"

StripeGenerator::StripeGenerator()
{
}

StripeGenerator::~StripeGenerator()
{
}

vector<Stripe> StripeGenerator::generateRandomStripes(ConvertibleCode &code, ClusterSettings &settings, mt19937 &random_generator) {
    size_t num_stripes = settings.N;
    size_t num_nodes = settings.M;

    vector<Stripe> stripes;

    for (size_t stripe_id = 0; stripe_id < num_stripes; stripe_id++) {
        vector<size_t> stripe_indices;
        
        // generate indices of stripe
        while (stripe_indices.size() < (size_t) code.n_i) {
            size_t random_node = Utils::randomUInt(0, num_nodes - 1, random_generator);

            if (find(stripe_indices.begin(), stripe_indices.end(), random_node) == stripe_indices.end()) {
                stripe_indices.push_back(random_node);
            }
        }

        Stripe stripe(stripe_id, stripe_indices);
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
        vector<size_t> &stripe_indices = stripe.getStripeIndices();

        for (auto idx : stripe_indices) {
            of << idx << " ";
        }

        of << endl;
    }

    of.close();
}

bool StripeGenerator::loadStripes(ConvertibleCode &code, ClusterSettings &settings, string placement_file, vector<Stripe> &stripes) {

    ifstream ifs(placement_file.c_str());

    if (ifs.fail()) {
        printf("invalid parameters\n");
        return false;
    }

    stripes.clear();

    string line;
    size_t stripe_id = 0;
    while (getline(ifs, line)) {
        istringstream iss(line);
        vector<size_t> stripe_indices;
        for (size_t block_id = 0; block_id < code.n_i; block_id++) {
            size_t idx;
            iss >> idx;
            stripe_indices.push_back(idx);
        }
        Stripe stripe(stripe_id, stripe_indices);
        stripes.push_back(stripe);
        stripe_id++;
    }

    ifs.close();

    return true;
}