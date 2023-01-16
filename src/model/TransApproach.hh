#ifndef __TRANS_APPROACH_HH__
#define __TRANS_APPROACH_HH__

#include "StripeGroup.hh"
#include "TransitionTask.hh"

enum TRANS_APCH {
    REENCODING,
    PARITY_MERGING,
    PARTIAL_PARITY_MERGING
};

class TransApproach
{
private:
    
public:
    TransApproach(/* args */);
    ~TransApproach();

    static vector<TransitionTask> getParityMergingTasks(StripeGroup *stripe_group);

};


#endif // __TRANS_APPROACH_HH__