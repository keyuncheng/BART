#ifndef __TRANSITION_TASK_HH__
#define __TRANSITION_TASK_HH__

#include "../include/include.hh"

typedef struct TransitionTask {
    int batch_id;
    int group_id;
    int stripe_id_group;
    int stripe_id_global;
    int block_id;
    int block_source_node;
    vector<int> block_target_node_list;
} TransitionTask;

#endif // __TRANSITION_TASK_HH__