#include "TransApproach.hh"

TransApproach::TransApproach(/* args */)
{
}

TransApproach::~TransApproach()
{
}

vector<TransitionTask> getParityMergingTasks(StripeGroup *stripe_group) {

    vector<TransitionTask> transition_tasks;

    ConvertibleCode code = stripe_group->getCode();
    ClusterSettings cluster_settings = stripe_group->getClusterSettings();
    int num_nodes = cluster_settings.M;

    // check parameters
    if (code.k_f < code.k_i || code.k_f % code.k_i != 0 || code.m_f > code.m_i) {
        printf("invalid code parameters\n");
        return transition_tasks;
    }

    // get the parity merging tasks
    vector<Stripe> &stripes = stripe_group->getStripes();
    vector<int> num_data_stored(num_nodes, 0);

    // check which node already stores at least one data block
    for (auto stripe : stripes) {
        vector<int> &stripe_indices = stripe.getStripeIndices();

        for (int block_id = 0; block_id < code.k_i; block_id++) {
            num_data_stored[stripe_indices[block_id]] += 1;
        }
    }

    // candidate nodes for data relocation
    vector<int> data_relocation_candidates;
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        if (num_data_stored[node_id] == 0) {
            data_relocation_candidates.push_back(node_id);
        }
    }

    // candidate nodes for parity relocation
    vector<int> parity_relocation_candidates = data_relocation_candidates;

    // data relocation tasks
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        if (num_data_stored[node_id] > 1) {
            for (int stripe_id = 1; stripe_id < code.alpha; stripe_id++) {
                vector<int> &stripe_indices = stripes[stripe_id].getStripeIndices();
                for (int block_id = 0; block_id < code.n_i; block_id++) {
                    // add a relocation task for this data block
                    if (stripe_indices[block_id] == node_id) {
                        TransitionTask task = {
                            .batch_id = -1,
                            .group_id = stripe_group->getId(),
                            .stripe_id_group = stripe_id,
                            .stripe_id_global = stripes[stripe_id].getId(),
                            .block_id = block_id,
                            .block_source_node = node_id,
                            .block_target_node_list = data_relocation_candidates
                        };
                        transition_tasks.push_back(task);
                    }
                }
            }
        }
    }

        // // parity merging tasks
        // for (int parity_id = 0; parity_id < code.m_f; parity_id++) {
        //     TransitionTask task = {
        //         .batch_id = -1,
        //         .group_id = stripe_group->getId(),
        //         .stripe_id_group = stripe_id,
        //         .stripe_id_global = stripes[stripe_id].getId(),
        //         .block_id = k_f + parity_id,
        //         .block_source_node = node_id,
        //         .block_target_node_list = data_relocation_candidates
        //     }
        // }


    return transition_tasks;
 }

