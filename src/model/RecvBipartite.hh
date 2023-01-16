#ifndef __RECV_BIPARTITE_HH__
#define __RECV_BIPARTITE_HH__

#include "Bipartite.hh"

class RecvBipartite : public Bipartite
{
private:
    /* data */
public:
    RecvBipartite(StripeBatch *stp_bth);
    ~RecvBipartite();
};


#endif // __RECV_BIPARTITE_HH__