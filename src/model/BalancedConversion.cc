#include "BalancdConversion.hh"

BalancdConversion::BalancdConversion(/* args */)
{
}

BalancdConversion::~BalancdConversion()
{
}

void BalancdConversion::getSolutionForStripeBatch(StripeBatch &stripe_batch, vector<vector<size_t> > &solutions, mt19937 random_generator) {
    ConvertibleCode &code = stripe_batch.getCode();
    ClusterSettings &settings = stripe_batch.getClusterSettings();

    vector<StripeGroup> &stripe_groups = stripe_batch.getStripeGroups();

    size_t num_stripe_groups = stripe_groups.size();

    // initialize solutions
    solutions.clear();

    // TODO: we don't need to generate real enumeration in vectors (can be represented by binary numbers)

    // Step 1: enumeration of approaches
    vector<vector<size_t> > approach_candidates;
    if (code.isValidForPM() == false) {
        // re-encoding only
        approach_candidates.push_back(vector<size_t>(num_stripe_groups, (size_t) TransApproach::RE_ENCODE));
    } else {
        // // parity merging only
        approach_candidates.push_back(vector<size_t>(num_stripe_groups, (size_t) TransApproach::PARITY_MERGE));

        // // re-encoding + parity merging (generate 2^num_sg enumerations)
        // approach_candidates = Utils::getEnumeration(num_stripe_groups, 2);
    }

    size_t num_approach_candidates = approach_candidates.size();

    printf("num_stripe_groups: %ld, number of approach candidates: %ld\n", num_stripe_groups, num_approach_candidates);

    // for (size_t app_cand_id = 0; app_cand_id < num_approach_candidates; app_cand_id++) {
    //     Utils::printUIntVector(approach_candidates[app_cand_id]);
    // }

    // Step 2: construct model for each approach candidate, and find the one with minimum max-load
    size_t best_app_cand_id = INVALID_ID;
    size_t best_max_load = INT_MAX;
    vector<vector<size_t> > best_solutions;

    for (size_t app_cand_id = 0; app_cand_id < num_approach_candidates; app_cand_id++) {
        vector<size_t> approach_candidate = approach_candidates[app_cand_id];

        printf("approach %ld:\n", app_cand_id);
        Utils::printUIntVector(approach_candidates[app_cand_id]);

        // Step 2.1: construct recv bipartite graph
        RecvBipartite recv_bipartite;
        recv_bipartite.constructStripeBatchWithApproaches(stripe_batch, approach_candidate);

        // recv_bipartite.print_block_metastore();
        // recv_bipartite.print();

        // find edges
        vector<size_t> edges;
        recv_bipartite.findEdgesWithApproachesGreedy(stripe_batch, edges, random_generator);

        // construct partial solutions from recv graph
        vector<vector<size_t> > partial_solutions, solutions;
        recv_bipartite.constructPartialSolutionFromEdges(stripe_batch, edges, partial_solutions);
        recv_bipartite.updatePartialSolutionFromRecvGraph(stripe_batch, partial_solutions, solutions);

        vector<size_t> send_load_dist, recv_load_dist;
        Utils::getLoadDist(code, settings, solutions, send_load_dist, recv_load_dist);

        // get bandwidth
        size_t bw_bt = 0;
        for (auto item : send_load_dist) {
            bw_bt += item;
        }

        size_t min_send_load = *min_element(send_load_dist.begin(), send_load_dist.end());
        size_t max_send_load = *max_element(send_load_dist.begin(), send_load_dist.end());
        size_t min_recv_load = *min_element(recv_load_dist.begin(), recv_load_dist.end());
        size_t max_recv_load = *max_element(recv_load_dist.begin(), recv_load_dist.end());
        

        printf("Balanced Transition (approach %ld) send load distribution:, minimum_load: %ld, maximum_load: %ld\n", app_cand_id,
            min_send_load, max_send_load);
        Utils::printUIntVector(send_load_dist);
        printf("Balanced Transition (approach %ld) recv load distribution:, minimum_load: %ld, maximum_load: %ld\n", app_cand_id,
            min_recv_load, max_recv_load);
        Utils::printUIntVector(recv_load_dist);
        printf("Balanced Transition (approach %ld) bandwidth: %ld\n", app_cand_id, bw_bt);

        size_t cand_max_load = max(max_send_load, max_recv_load);
        if (cand_max_load < best_max_load) {
            best_max_load = cand_max_load;
            best_app_cand_id = app_cand_id;
            best_solutions = solutions;
        }
    }

    if (best_app_cand_id != INVALID_ID) {
        printf("found best candidate id: %ld, best_max_load: %ld, approaches:\n", best_app_cand_id, best_max_load);
        Utils::printUIntVector(approach_candidates[best_app_cand_id]);
        solutions = best_solutions;
    }

    // // put it into the solution for the batch
    // printf("solutions for stripe_batch (size: %ld): \n", solutions.size());
    // for (auto solution : solutions) {
    //     printf("stripe %ld, block %ld, from: %ld, to: %ld\n", solution[0], solution[1], solution[2], solution[3]);
    // }
}