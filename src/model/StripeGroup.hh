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
    void initDataDist();
    void initParityDists();

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
    void print();

    uint8_t getMinTransBW(string approach);
    uint8_t getDataRelocBW();
    uint8_t getMinREBW();
    uint8_t getMinPMBW();
    uint8_t getMinPMBWGreedy();

    // generate parity computation scheme for perfect parity merging (parity generation bandwidth = 0)
    void genParityComputeScheme4PerfectPM();

    // generate candidate partial load tables for the stripe group with re-encoding and parity merging
    void genPartialLTs4ParityCompute(string approach);

    // // enumerate send load tables
    // vector<vector<size_t>> getCandSendLoadTables();
    // int constructInitSLTWithDataRelocation(vector<size_t> &send_load_table);
    // void appendCandSLTsWithParityMerging(vector<size_t> &init_slt, vector<vector<size_t>> &cand_slts);
    // void appendCandSLTsWithReEncoding(vector<size_t> &init_slt, vector<vector<size_t>> &cand_slts);

    // vector<LoadTable> getCandSLTs();
    // LoadTable getPartialSLTWithDataRelocation();
    // LoadTable getPartialSLTWithReEncoding();
    // LoadTable getPartialSLTWIthParityMerging();

    // // get min cost send load tables
    // vector<vector<size_t>> getCandSendLoadTablesForMinTransCost(int min_cost);
    // int appendMinCostSLTWithParityMerging(vector<size_t> &send_load_table, vector<vector<size_t>> &cand_send_load_tables, int min_cost);
    // int appendMinCostSLTWithReEncoding(vector<size_t> &init_send_load_table, vector<vector<size_t>> &cand_send_load_tables, int min_cost);
};

#endif // __STRIPE_GROUP_HH__