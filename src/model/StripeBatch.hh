#ifndef __STRIPE_BATCH_HH__
#define __STRIPE_BATCH_HH__

#include "../include/include.hh"
#include "../util/Utils.hh"
#include "StripeGroup.hh"

class StripeBatch
{
private:
public:
    uint8_t id;
    ConvertibleCode &code;
    ClusterSettings &settings;
    mt19937 &random_generator;
    vector<Stripe> pre_stripes;  // placement of pre-transition stripes
    vector<Stripe> post_stripes; // placement of post-transition stripes

    // step 1: stripe group selection
    map<uint32_t, StripeGroup> selected_sgs; // selected stripe groups in order <sg_id, StripeGroup>

    StripeBatch(uint8_t _id, ConvertibleCode &_code, ClusterSettings &_settings, mt19937 &_random_generator);
    ~StripeBatch();

    /**
     * @brief construct stripe group in sequence of stripes
     *
     */
    void constructSGInSequence();

    /**
     * @brief construct stripe group by random selection of stripes
     *
     */
    void constructSGByRandomPick();

    /**
     * @brief construct stripe group by bandwidth
     * we directly enumerate all possible stripe groups with size lambda
     *
     * @param approach
     */
    void constructSGByBWBF(string approach);

    /**
     * @brief construct stripe group by bandwidth (with partial stripe groups)
     * we construct stripe groups starting from building 2-partial stripe
     * groups; then increase their sizes
     *
     * @param approach
     */
    void constructSGByBWPartial(string approach);

    /**
     * @brief store selected stripe group metadata into sg_meta_filename
     *
     * format: <pre_stripe_ids: lambda>, <encoding method>, <encoding nodes: m_f>
     *
     * @param sg_meta_filename
     */
    void storeSGMetadata(string sg_meta_filename);

    /**
     * @brief load selected stripe group metadata from sg_meta_filename
     *
     * format: <pre_stripe_ids: lambda>, <encoding method>, <encoding nodes: m_f>
     *
     * @param sg_meta_filename
     * @return true
     * @return false
     */
    bool loadSGMetadata(string sg_meta_filename);

    /**
     * @brief print stripe batch
     *
     */
    void print();
};

#endif // __STRIPE_BATCH_HH__