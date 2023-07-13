#ifndef __STRIPE_GROUP_HH__
#define __STRIPE_GROUP_HH__

#include "../include/include.hh"
#include "../util/Utils.hh"
#include "ConvertibleCode.hh"
#include "ClusterSettings.hh"
#include "Stripe.hh"

enum EncodeMethod
{
    RE_ENCODE,
    PARITY_MERGE,
    UNKNOWN_METHOD
};

typedef struct LoadTable
{
    EncodeMethod approach;
    u16string enc_nodes; // parity generation nodes (m parity blocks)

    u16string slt; // send load table
    u16string rlt; // receive load table
    uint32_t bw;   // bandwidth

    LoadTable()
    {
        approach = EncodeMethod::UNKNOWN_METHOD;
        bw = 0;
    }
} LoadTable;

class StripeGroup
{

private:
public:
    uint32_t id;
    ConvertibleCode &code;
    ClusterSettings &settings;

    // stripes
    vector<Stripe *> pre_stripes; // pre-transition stripes
    Stripe *post_stripe;          // post transition stripes

    // block distributions (data and parity)
    u16string data_dist;
    vector<u16string> parity_dists;

    // nodes for parity block computation
    EncodeMethod parity_comp_method;
    u16string parity_comp_nodes;

    // (for BalancedConversion only) load tables
    LoadTable applied_lt;
    vector<LoadTable> cand_partial_lts;

    StripeGroup(uint32_t _id, ConvertibleCode &_code, ClusterSettings &_settings, vector<Stripe *> &_pre_stripes, Stripe *_post_stripe);
    ~StripeGroup();

    /**
     * @brief print stripe group
     *
     */
    void print();

    /**
     * @brief initialize data distribution
     *
     */
    void initDataDist();

    /**
     * @brief initialize parity distributions
     *
     */
    void initParityDists();

    /**
     * @brief Get transitioning bandwidth for the stripe group
     *
     * @param approach transitioning approach (re-encoding, parity merging)
     * @param enc_nodes encoding node for assignment
     * @return uint8_t bandwidth
     */
    uint8_t getTransBW(string approach, u16string &enc_nodes);

    /**
     * @brief Get data relocation bandwidth
     *
     * @return uint8_t bandwidth
     */
    uint8_t getDataRelocBW();

    /**
     * @brief get re-encoding bandwidth
     *
     * @param enc_nodes
     * @return uint8_t
     */
    uint8_t getREBW(u16string &enc_nodes);

    /**
     * @brief get parity merging bandwidth
     *
     * @param enc_nodes
     * @return uint8_t
     */
    uint8_t getPMBW(u16string &enc_nodes);

    bool isPerfectParityMerging();

    // generate parity computation scheme for perfect parity merging (parity generation bandwidth = 0)
    void genParityComputeScheme4PerfectPM();

    // generate candidate partial load tables for the stripe group with re-encoding and parity merging
    void genPartialLTs4ParityCompute(string approach);

    LoadTable genPartialLT4ParityCompute(EncodeMethod enc_method, u16string enc_nodes);
};

#endif // __STRIPE_GROUP_HH__