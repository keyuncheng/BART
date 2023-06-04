#include "StripeGenerator.hh"

StripeGenerator::StripeGenerator()
{
}

StripeGenerator::~StripeGenerator()
{
}

void StripeGenerator::genRandomStripes(ConvertibleCode &code, ClusterSettings &settings, mt19937 &random_generator, vector<Stripe> &stripes)
{
    u16string nodes(settings.num_nodes, 0);
    for (uint16_t node_id = 0; node_id < settings.num_nodes; node_id++)
    {
        nodes[node_id] = node_id;
    }

    for (uint32_t stripe_id = 0; stripe_id < settings.num_stripes; stripe_id++)
    {
        // set stripe
        Stripe &stripe = stripes[stripe_id];
        // random order the indices
        u16string sorted_nodes = nodes;
        shuffle(sorted_nodes.begin(), sorted_nodes.end(), random_generator);
        stripe.indices = sorted_nodes.substr(0, code.n_i);
    }
}

void StripeGenerator::storeStripes(vector<Stripe> &stripes, string placement_filename)
{
    if (stripes.size() == 0)
    {
        printf("invalid parameters\n");
        return;
    }

    ofstream of;

    of.open(placement_filename, ios::out);

    for (auto stripe : stripes)
    {

        for (auto idx : stripe.indices)
        {
            of << idx << " ";
        }
        of << endl;
    }

    of.close();

    printf("finished storing %lu stripes in %s\n", stripes.size(), placement_filename.c_str());
}

bool StripeGenerator::loadStripes(ConvertibleCode &code, ClusterSettings &settings, string placement_filename, vector<Stripe> &stripes)
{
    ifstream ifs(placement_filename.c_str());

    if (ifs.fail())
    {
        printf("invalid placement file: %s\n", placement_filename.c_str());
        return false;
    }

    string line;
    uint32_t stripe_id = 0;
    while (getline(ifs, line))
    {
        // set stripe
        Stripe &stripe = stripes[stripe_id];

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

    printf("finished loading %lu stripes from %s\n", stripes.size(), placement_filename.c_str());

    return true;
}

bool StripeGenerator::loadBlockMapping(ConvertibleCode &code, ClusterSettings &settings, string block_mapping_filename, vector<vector<pair<uint16_t, string>>> &stripe_placements)
{
    ifstream ifs(block_mapping_filename.c_str());

    if (ifs.fail())
    {
        printf("invalid block mapping file: %s\n", block_mapping_filename.c_str());
        return false;
    }

    // init stripe placement
    stripe_placements.clear();
    stripe_placements.assign(settings.num_stripes, vector<pair<uint16_t, string>>(code.n_i, pair<uint16_t, string>(0, "")));

    string line;
    uint64_t record_id = 0;
    while (getline(ifs, line))
    {
        // set stripe
        uint32_t stripe_id = INVALID_STRIPE_ID_GLOBAL;
        uint8_t block_id = INVALID_BLK_ID;
        uint16_t placed_node_id = INVALID_NODE_ID;
        string placed_path = "";

        istringstream iss(line);
        unsigned int val;

        iss >> val;
        stripe_id = val; // get stripe_id

        iss >> val;
        block_id = val; // get block_id

        iss >> val;
        placed_node_id = val; // get placed_node_id

        iss >> placed_path; // get placed_path

        stripe_placements[stripe_id][block_id].first = placed_node_id;
        stripe_placements[stripe_id][block_id].second = placed_path;

        record_id++;
    }

    ifs.close();

    // for (uint32_t stripe_id = 0; stripe_id < settings.num_stripes; stripe_id++)
    // {
    //     for (uint8_t block_id = 0; block_id < code.n_i; block_id++)
    //     {
    //         printf("stripe_id: %u, block_id: %u, placed_node_id: %u, placed_path: %s\n", stripe_id, block_id, stripe_placements[stripe_id][block_id].first, stripe_placements[stripe_id][block_id].second.c_str());
    //     }
    // }

    printf("finished loading %ld blocks metadata from %s\n", record_id, block_mapping_filename.c_str());

    return true;
}