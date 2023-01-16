#ifndef __RECV_BIPARTITE_HH__
#define __RECV_BIPARTITE_HH__

#include "Bipartite.hh"

class RecvBipartite : public Bipartite
{
private:

public:
    RecvBipartite();
    ~RecvBipartite();

    bool addStripeBatch(StripeBatch &stripe_batch);
    bool addStripeGroup(StripeGroup &stripe_group);
};


#endif // __RECV_BIPARTITE_HH__