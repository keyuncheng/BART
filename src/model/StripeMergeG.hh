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
    StripeMergeG(/* args */);
    ~StripeMergeG();

    void getSolutionForStripeBatch(StripeBatch &stripe_batch, vector<vector<int>> &solutions);
    void getSolutionForStripeGroup(StripeGroup &stripe_group, vector<vector<int>> &solutions);

    void getLoadDist(ConvertibleCode &code, ClusterSettings &settings, vector<vector<int>> &solutions, vector<int> &load_dist);
};




#endif