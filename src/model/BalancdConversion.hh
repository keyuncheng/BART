#ifndef __BALANCED_CONVERSION_HH__
#define __BALANCED_CONVERSION_HH__

#include "../util/Utils.hh"
#include "StripeBatch.hh"
#include "RecvBipartite.hh"


class BalancdConversion
{
private:
    /* data */
public:
    BalancdConversion(/* args */);
    ~BalancdConversion();

    void getSolutionForStripeBatchGlobal(StripeBatch &stripe_batch, vector<vector<size_t> > &solutions, mt19937 random_generator);

    void getSolutionForStripeBatchGreedy(StripeBatch &stripe_batch, vector<vector<size_t> > &solutions, mt19937 random_generator);

    void getSolutionForStripeBatchIter(StripeBatch &stripe_batch, vector<vector<size_t> > &solutions, mt19937 random_generator);

    void getLoadDist(ConvertibleCode &code, ClusterSettings &settings, vector<vector<size_t> > &solutions, vector<size_t> &send_load_dist, vector<size_t> &recv_load_dist);
};



#endif // __BALANCED_CONVERSION_HH__