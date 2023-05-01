#include "StripeGenerator.hh"

StripeGenerator::StripeGenerator()
{
}

StripeGenerator::~StripeGenerator()
{
}

void StripeGenerator::genRandomStripes(ConvertibleCode &code, ClusterSettings &settings, mt19937 &random_generator, vector<Stripe> &stripes)
{

    // init stripes
    stripes.clear();
    stripes.assign(settings.num_stripes, Stripe());

    u16string nodes(settings.num_nodes, 0);
    for (uint16_t node_id = 0; node_id < settings.num_nodes; node_id++)
    {
        nodes[node_id] = node_id;
    }

    for (uint stripe_id = 0; stripe_id < settings.num_stripes; stripe_id++)
    {
        // set stripe
        Stripe &stripe = stripes[stripe_id];
        stripe.id = stripe_id;
        // random order the indices
        u16string sorted_nodes = nodes;
        shuffle(sorted_nodes.begin(), sorted_nodes.end(), random_generator);
        stripe.indices = sorted_nodes.substr(0, code.n_i);
    }
}

void StripeGenerator::storeStripes(vector<Stripe> &stripes, string placement_file)
{
    if (stripes.size() == 0)
    {
        printf("invalid parameters\n");
        return;
    }

    ofstream of;

    of.open(placement_file, ios::out);

    for (auto stripe : stripes)
    {

        for (auto idx : stripe.indices)
        {
            of << idx << " ";
        }
        of << endl;
    }

    of.close();
}

bool StripeGenerator::loadStripes(ConvertibleCode &code, ClusterSettings &settings, string placement_file, vector<Stripe> &stripes)
{
    ifstream ifs(placement_file.c_str());

    if (ifs.fail())
    {
        printf("invalid parameters\n");
        return false;
    }

    // init stripes
    stripes.clear();
    stripes.assign(settings.num_stripes, Stripe());

    string line;
    uint stripe_id = 0;
    while (getline(ifs, line))
    {
        // set stripe
        Stripe &stripe = stripes[stripe_id];
        stripe.id = stripe_id;
        stripe.indices.assign(code.n_i, 0);

        // get indices
        istringstream iss(line);
        for (uint8_t block_id = 0; block_id < code.n_i; block_id++)
        {
            uint16_t idx;
            iss >> idx;
            stripe.indices[block_id] = idx;
        }

        stripe_id++;
    }

    ifs.close();

    return true;
}