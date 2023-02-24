#include "BalancdConversion.hh"

BalancdConversion::BalancdConversion(/* args */)
{
}

BalancdConversion::~BalancdConversion()
{
}

void BalancdConversion::getSolutionForStripeBatch(StripeBatch &stripe_batch, vector<vector<int> > &solutions, mt19937 random_generator) {
    ConvertibleCode &code = stripe_batch.getCode();
    ClusterSettings &settings = stripe_batch.getClusterSettings();

    vector<StripeGroup> &stripe_groups = stripe_batch.getStripeGroups();

    int num_stripe_groups = stripe_groups.size();

    // initialize solutions
    solutions.clear();

    // TODO: we don't need to generate real enumeration in vectors (can be represented by binary numbers)

    // Step 1: enumeration of approaches
    vector<vector<int> > approach_candidates;
    if (code.isValidForPM() == false) {
        // re-encoding only
        approach_candidates.push_back(vector<int>(num_stripe_groups, TransApproach::RE_ENCODE));
    } else {
        // parity merging only
        approach_candidates.push_back(vector<int>(num_stripe_groups, TransApproach::PARITY_MERGE));

        // re-encoding + parity merging (generate 2^num_sg enumerations)
        approach_candidates = Utils::getEnumeration(num_stripe_groups, 2);
    }

    int num_approach_candidates = approach_candidates.size();

    printf("num_stripe_groups: %d, number of approach candidates: %d\n", num_stripe_groups, num_approach_candidates);

    // for (int app_cand_id = 0; app_cand_id < num_approach_candidates; app_cand_id++) {
    //     Utils::printIntVector(approach_candidates[app_cand_id]);
    // }

    // Step 2: construct model for each approach candidate, and find the one with minimum max-load
    int best_app_cand_id = -1;
    int best_max_load = INT_MAX;
    vector<vector<int> > best_solutions;

    for (int app_cand_id = 0; app_cand_id < num_approach_candidates; app_cand_id++) {
        vector<int> approach_candidate = approach_candidates[app_cand_id];

        printf("approach %d:\n", app_cand_id);
        Utils::printIntVector(approach_candidates[app_cand_id]);

        // Step 2.1: construct recv bipartite graph
        RecvBipartite recv_bipartite;
        recv_bipartite.constructStripeBatchWithApproaches(stripe_batch, approach_candidate);

        // recv_bipartite.print_meta();
        // recv_bipartite.print();

        // find edges
        vector<int> edges;
        recv_bipartite.findEdgesWithApproachesGreedy(stripe_batch, edges, random_generator);

        // construct partial solutions from recv graph
        vector<vector<int> > partial_solutions;
        recv_bipartite.constructPartialSolutionFromEdges(stripe_batch, edges, partial_solutions);

        vector<vector<int> > cand_solutions;
        SendBipartite send_bipartite;
        send_bipartite.updatePartialSolutionFromRecvGraph(stripe_batch, partial_solutions, cand_solutions);

        vector<int> send_load_dist, recv_load_dist;
        Utils::getLoadDist(code, settings, cand_solutions, send_load_dist, recv_load_dist);

        // get bandwidth
        int bw_bt = 0;
        for (auto item : send_load_dist) {
            bw_bt += item;
        }

        int min_send_load = *min_element(send_load_dist.begin(), send_load_dist.end());
        int max_send_load = *max_element(send_load_dist.begin(), send_load_dist.end());
        int min_recv_load = *min_element(recv_load_dist.begin(), recv_load_dist.end());
        int max_recv_load = *max_element(recv_load_dist.begin(), recv_load_dist.end());
        

        printf("Balanced Transition (approach %d) send load distribution:, minimum_load: %d, maximum_load: %d\n", app_cand_id,
            min_send_load, max_send_load);
        Utils::printIntVector(send_load_dist);
        printf("Balanced Transition (approach %d) recv load distribution:, minimum_load: %d, maximum_load: %d\n", app_cand_id,
            min_recv_load, max_recv_load);
        Utils::printIntVector(recv_load_dist);
        printf("Balanced Transition (approach %d) bandwidth: %d\n", app_cand_id, bw_bt);

        int cand_max_load = max(max_send_load, max_recv_load);
        if (cand_max_load < best_max_load) {
            best_max_load = cand_max_load;
            best_app_cand_id = app_cand_id;
            best_solutions = cand_solutions;
        }
    }

    if (best_app_cand_id != -1) {
        printf("found best candidate id: %d, best_max_load: %d, approaches:\n", best_app_cand_id, best_max_load);
        Utils::printIntVector(approach_candidates[best_app_cand_id]);
        solutions = best_solutions;
    }

    // // put it into the solution for the batch
    // printf("solutions for stripe_batch (size: %ld): \n", solutions.size());
    // for (auto solution : solutions) {
    //     printf("stripe %d, block %d, from: %d, to: %d\n", solution[0], solution[1], solution[2], solution[3]);
    // }
}