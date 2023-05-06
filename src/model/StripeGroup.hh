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
    u16string re_nodes; // parity generation nodes for re-encoding
    u16string pm_nodes; // parity generation nodes for parity merging

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
    vector<Stripe *> sg_stripes;

    // block distributions (data and parity)
    u16string data_dist;
    vector<u16string> parity_dists;

    // for BalancedConversion
    LoadTable applied_lt;
    vector<LoadTable> cand_partial_lts;

    StripeGroup(uint32_t _id, ConvertibleCode &_code, ClusterSettings &_settings, vector<Stripe *> &_sg_stripes);
    ~StripeGroup();
    void print();

    uint8_t getMinTransBW();
    uint8_t getDataRelocBW();
    uint8_t getMinREBW();
    uint8_t getMinPMBW();

    // generate parity generation scheme for perfect parity merging (parity generation bandwidth = 0)
    void genParityGenScheme4PerfectPM();

    // generate all candidate partial load tables for the stripe group, including re-encoding and parity merging
    void genAllPartialLTs4ParityGen();

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