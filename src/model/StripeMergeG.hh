#ifndef __STRIPE_MERGE_G_HH__
#define __STRIPE_MERGE_G_HH__

#include "../include/include.hh"
#include "../util/Utils.hh"
#include "StripeBatch.hh"
#include "ConvertibleCode.hh"
#include "ClusterSettings.hh"

class StripeMergeG
{
private:
    /* data */
public:
    StripeMergeG();
    ~StripeMergeG();

    void getSolutionForStripeBatch(StripeBatch &stripe_batch, vector<vector<int> > &solutions, mt19937 random_generator);
    void getSolutionForStripeGroup(StripeGroup &stripe_group, vector<vector<int> > &solutions, mt19937 random_generator);

    void getLoadDist(ConvertibleCode &code, ClusterSettings &settings, vector<vector<int> > &solutions, vector<int> &send_load_dist, vector<int> &recv_load_dist);
};




#endif