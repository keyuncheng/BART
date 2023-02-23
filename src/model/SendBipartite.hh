#ifndef __SEND_BIPARTITE_HH__
#define __SEND_BIPARTITE_HH__

#include "StripeBatch.hh"
#include "StripeGroup.hh"
#include "RecvBipartite.hh"


class SendBipartite : public Bipartite
{
private:
    /* data */
public:
    SendBipartite(/* args */);
    ~SendBipartite();

    bool addStripeBatchFromRecvGraph(StripeBatch &stripe_batch,  RecvBipartite &recv_bipartite);
    bool addStripeGroupFromRecvGraph(StripeGroup &stripe_group, RecvBipartite &recv_bipartite);

    void print();

    // construct solution based on recv graph with approaches
    bool updatePartialSolutionFromRecvGraph(StripeBatch &stripe_batch, vector<vector<int> > &partial_solutions, vector<vector<int> > &solutions);

private:
    bool addStripeGroupWithDataFromRecvGraph(StripeGroup &stripe_group, RecvBipartite &recv_bipartite);
    bool addStripeGroupWithParityMergingFromRecvGraph(StripeGroup &stripe_group, RecvBipartite &recv_bipartite);
    bool addStripeGroupWithReEncodingFromRecvGraph(StripeGroup &stripe_group, RecvBipartite &recv_bipartite);
    bool addStripeGroupWithPartialParityMergingFromRecvGraph(StripeGroup &stripe_group, RecvBipartite &recv_bipartite);

};




#endif // __SEND_BIPARTITE_HH__
