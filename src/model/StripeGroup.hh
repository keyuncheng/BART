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
    PARITY_MERGE
};

// typedef struct LoadTable
// {
//     EncodeMethod approach;
//     vector<size_t> lt; // load table
//     size_t cost;

//     LoadTable()
//     {
//         approach = EncodeMethod::RE_ENCODE;
//         lt.clear();
//         cost = 0;
//     }
// } LoadTable;

class StripeGroup
{

private:
    void initDataDist();
    void initParityDists();

    void initMinTransBW();
    uint8_t getDataRelocBW();
    uint8_t getMinREBW();
    uint8_t getMinPMBW();

public:
    size_t id;
    ConvertibleCode &code;
    ClusterSettings &settings;
    vector<Stripe *> stripes;

    // block distributions (data and parity)
    u16string data_dist;
    vector<u16string> parity_dists;

    uint8_t min_bw;                 // mininum bandwidth
    EncodeMethod min_bw_enc_method; // encoding method for mininum bandwidth

    StripeGroup(size_t _id, ConvertibleCode &_code, ClusterSettings &_settings, vector<Stripe *> &_stripes);
    ~StripeGroup();
    void print();

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