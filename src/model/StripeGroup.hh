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

    // (for BART only) load tables
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

    /**
     * @brief check if the parity block can be perfectly merged (zero
     * bandwidth), which requires that the parity blocks all reside in the
     * same node
     *
     * @param parity_id
     * @return true
     * @return false
     */
    bool isPerfectPM(uint8_t parity_id);

    /**
     * @brief check if all code.m_f parity block can be perfectly merged
     *
     * @return true
     * @return false
     */
    bool isPerfectPM();

    /**
     * @brief get the encoding node where all the parity blocks all reside for
     * perfect parity merging
     *
     * @param parity_id
     * @return uint16_t
     */
    uint16_t genEncNodeForPerfectPM(uint8_t parity_id);

    /**
     * @brief generate the load table for perfect parity merging, where all
     * code.m_f parity blocks can be perfectly merged
     *
     */
    void genLTForPerfectPM();

    /**
     * @brief generate partial load table with traffic for parity generation
     * only; the encoding nodes and enc_method specifies the computation scheme
     *
     * @param enc_method
     * @param enc_nodes
     * @return LoadTable
     */
    LoadTable genPartialLTForParityCompute(EncodeMethod enc_method, u16string &enc_nodes);

    /**
     * @brief generate all possible partial load tables with traffic for
     * parity generation only
     *
     * @param approach transitioning approach
     * @return LoadTable
     */
    void genAllPartialLTs4ParityCompute(string approach);
};

#endif // __STRIPE_GROUP_HH__