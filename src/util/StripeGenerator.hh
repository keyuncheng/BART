#ifndef __STRIPE_GENERATOR_HH__
#define __STRIPE_GENERATOR_HH__

#include <fstream>
#include <sstream>

#include "../include/include.hh"
#include "../model/ConvertibleCode.hh"
#include "../model/ClusterSettings.hh"
#include "../model/Stripe.hh"
#include "../util/Utils.hh"

class StripeGenerator
{
private:
public:
    StripeGenerator();
    StripeGenerator(mt19937 &random_generator);
    ~StripeGenerator();

    void genRandomStripes(ConvertibleCode &code, ClusterSettings &settings, mt19937 &random_generator, vector<Stripe> &stripes);

    void storeStripes(vector<Stripe> &stripes, string placement_filename);
    bool loadStripes(ConvertibleCode &code, ClusterSettings &settings, string placement_filename, vector<Stripe> &stripes);

    /**
     * @brief read block mapping file
     *
     * @param code
     * @param settings
     * @param block_mapping_filename
     * @param stripe_placements <stripe <block> <placed_node_id, placed_path>>
     * @return true
     * @return false
     */
    bool loadBlockMapping(ConvertibleCode &code, ClusterSettings &settings, string block_mapping_filename, vector<vector<pair<uint16_t, string>>> &stripe_placements);
};

#endif // __STRIPE_GENERATOR_HH__