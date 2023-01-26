#ifndef __RECV_BIPARTITE_HH__
#define __RECV_BIPARTITE_HH__

#include "Bipartite.hh"
#include "StripeMeta.hh"
#include "../util/Utils.hh"

class RecvBipartite : public Bipartite
{
private:

public:
    RecvBipartite();
    ~RecvBipartite();

    bool addStripeBatch(StripeBatch &stripe_batch);
    bool addStripeGroup(StripeGroup &stripe_group);

    void print();

private:
    bool addStripeGroupWithData(StripeGroup &stripe_group);
    bool addStripeGroupWithParityMerging(StripeGroup &stripe_group);
    bool addStripeGroupWithReEncoding(StripeGroup &stripe_group);
    bool addStripeGroupWithPartialParityMerging(StripeGroup &stripe_group);

    map<int, BlockMeta> block_meta_map; // block metadata
    map<int, NodeMeta> node_meta_map; // node metadata

};


#endif // __RECV_BIPARTITE_HH__