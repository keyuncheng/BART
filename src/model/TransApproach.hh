#ifndef __TRANS_APPROACH_HH__
#define __TRANS_APPROACH_HH__

#include "StripeGroup.hh"

enum TRANS_APCH {
    REENCODING,
    PARITY_MERGING,
    PARTIAL_PARITY_MERGING
};

class TransApproach
{
private:
    StripeGroup *_stripe_group;
    TRANS_APCH _approach; // approach id

    
public:
    TransApproach(/* args */);
    ~TransApproach();
};


#endif // __TRANS_APPROACH_HH__