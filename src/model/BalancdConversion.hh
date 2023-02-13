#ifndef __BALANCED_CONVERSION_HH__
#define __BALANCED_CONVERSION_HH__

#include "../util/Utils.hh"
#include "StripeBatch.hh"
#include "RecvBipartite.hh"
#include "SendBipartite.hh"


class BalancdConversion
{
private:
    /* data */
public:
    BalancdConversion(/* args */);
    ~BalancdConversion();

    void getSolutionForStripeBatch(StripeBatch &stripe_batch, vector<vector<int> > &solutions, mt19937 random_generator);

    void getLoadDist(ConvertibleCode &code, ClusterSettings &settings, vector<vector<int> > &solutions, vector<int> &send_load_dist, vector<int> &recv_load_dist);
};



#endif // __BALANCED_CONVERSION_HH__