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

bool StripeGenerator::loadStripes(uint8_t ecn, string placement_filename, vector<Stripe> &stripes)
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
        for (uint8_t block_id = 0; block_id < ecn; block_id++)
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

bool StripeGenerator::loadBlockMapping(uint8_t ecn, uint32_t num_stripes, string block_mapping_filename, vector<vector<pair<uint16_t, string>>> &block_mapping)
{
    ifstream ifs(block_mapping_filename.c_str());

    if (ifs.fail())
    {
        printf("invalid block mapping file: %s\n", block_mapping_filename.c_str());
        return false;
    }

    // init block mapping
    block_mapping.clear();
    block_mapping.assign(num_stripes, vector<pair<uint16_t, string>>(ecn, pair<uint16_t, string>(0, "")));

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

        block_mapping[stripe_id][block_id].first = placed_node_id;
        block_mapping[stripe_id][block_id].second = placed_path;

        record_id++;
    }

    ifs.close();

    if (record_id != ecn * num_stripes)
    {
        fprintf(stderr, "error: invalid number of records %lu\n", record_id);
        exit(EXIT_FAILURE);
    }

    // for (uint32_t stripe_id = 0; stripe_id < num_stripes; stripe_id++)
    // {
    //     for (uint8_t block_id = 0; block_id < ecn; block_id++)
    //     {
    //         printf("stripe_id: %u, block_id: %u, placed_node_id: %u, placed_path: %s\n", stripe_id, block_id, block_mapping[stripe_id][block_id].first, block_mapping[stripe_id][block_id].second.c_str());
    //     }
    // }

    printf("finished loading %ld blocks metadata from %s\n", record_id, block_mapping_filename.c_str());

    return true;
}