#include "BalancdConversion.hh"

BalancdConversion::BalancdConversion(/* args */)
{
}

BalancdConversion::~BalancdConversion()
{
}

void BalancdConversion::getSolutionForStripeBatchGlobal(StripeBatch &stripe_batch, vector<vector<size_t> > &solutions, mt19937 random_generator) {
    ConvertibleCode &code = stripe_batch.getCode();
    ClusterSettings &settings = stripe_batch.getClusterSettings();

    vector<StripeGroup> &stripe_groups = stripe_batch.getStripeGroups();

    size_t num_stripe_groups = stripe_groups.size();

    // initialize solutions
    solutions.clear();

    // Step 1: enumeration of approaches
    vector<vector<size_t> > approach_candidates;
    if (code.isValidForPM() == false) {
        // re-encoding only
        approach_candidates.push_back(vector<size_t>(num_stripe_groups, (size_t) TransApproach::RE_ENCODE));
    } else {
        // parity merging only
        // approach_candidates.push_back(vector<size_t>(num_stripe_groups, (size_t) TransApproach::PARITY_MERGE));

        // re-encoding + parity merging (generate 2^num_sg enumerations)
        approach_candidates = Utils::getEnumeration(num_stripe_groups, 2);
    }

    size_t num_approach_candidates = approach_candidates.size();

    printf("num_stripe_groups: %ld, number of approach candidates: %ld\n", num_stripe_groups, num_approach_candidates);

    // for (size_t app_cand_id = 0; app_cand_id < num_approach_candidates; app_cand_id++) {
    //     Utils::printUIntVector(approach_candidates[app_cand_id]);
    // }

    // Step 2: construct model for each approach candidate, and find the one with minimum max-load
    size_t best_app_cand_id = INVALID_ID;
    size_t best_max_load = SIZE_MAX;
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
        vector<size_t> sol_edges;
        recv_bipartite.findEdgesWithApproachesGreedy(stripe_batch, sol_edges, random_generator);

        // construct partial solutions from recv graph
        vector<vector<size_t> > partial_solutions, solutions;
        recv_bipartite.constructPartialSolutionFromEdges(stripe_batch, sol_edges, partial_solutions);
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

void BalancdConversion::getSolutionForStripeBatchGreedy(StripeBatch &stripe_batch, vector<vector<size_t> > &solutions, mt19937 random_generator) {
    ConvertibleCode &code = stripe_batch.getCode();
    ClusterSettings &settings = stripe_batch.getClusterSettings();

    vector<StripeGroup> &stripe_groups = stripe_batch.getStripeGroups();

    size_t num_stripe_groups = stripe_groups.size();

    // initialize solutions
    solutions.clear();

    StripeBatch cur_stripe_batch = stripe_batch;
    vector<StripeGroup> &cur_stripe_group = cur_stripe_batch.getStripeGroups();
    cur_stripe_group.clear();
    vector<size_t> cur_approaches;
    vector<vector<size_t> > cur_solutions;

    // Step 1: for each stripe group, greedily finds the most load-balanced approach
    for (size_t sg_id = 0; sg_id < num_stripe_groups; sg_id++) {
        // add group sg_id to current stripe batch
        cur_stripe_group.push_back(stripe_groups[sg_id]);

        // Step 2: construct model for each approach candidate, and find the one with minimum max-load
        size_t best_approach_id = INVALID_ID;
        size_t best_max_load = SIZE_MAX;
        vector<vector<size_t> > best_solutions;

        for (size_t cand_approach_id = 0; cand_approach_id < 2; cand_approach_id++) {
            vector<size_t> cand_approaches = cur_approaches;
            cand_approaches.push_back(cand_approach_id);

            // construct recv bipartite graph
            RecvBipartite cur_recv_bipartite;
            cur_recv_bipartite.constructStripeBatchWithApproaches(cur_stripe_batch, cand_approaches);

            // find edges
            vector<size_t> cand_sol_edges;
            cur_recv_bipartite.findEdgesWithApproachesGreedy(cur_stripe_batch, cand_sol_edges, random_generator);

            // construct partial solutions from recv graph
            vector<vector<size_t> > cand_partial_solutions, cand_solutions;
            cur_recv_bipartite.constructPartialSolutionFromEdges(cur_stripe_batch, cand_sol_edges, cand_partial_solutions);
            cur_recv_bipartite.updatePartialSolutionFromRecvGraph(cur_stripe_batch, cand_partial_solutions, cand_solutions);

            vector<size_t> send_load_dist, recv_load_dist;
            Utils::getLoadDist(code, settings, cand_solutions, send_load_dist, recv_load_dist);

            // size_t min_send_load = *min_element(send_load_dist.begin(), send_load_dist.end());
            size_t max_send_load = *max_element(send_load_dist.begin(), send_load_dist.end());
            // size_t min_recv_load = *min_element(recv_load_dist.begin(), recv_load_dist.end());
            size_t max_recv_load = *max_element(recv_load_dist.begin(), recv_load_dist.end());
        

            size_t cand_max_load = max(max_send_load, max_recv_load);
            if (cand_max_load < best_max_load) {
                best_max_load = cand_max_load;
                best_approach_id = cand_approach_id;
                best_solutions = cand_solutions;
            }
        }

        if (best_approach_id != INVALID_ID) {
            printf("stripe_group %ld, best approach: %ld, best_max_load: %ld, approaches:\n", sg_id, best_approach_id, best_max_load);
            cur_approaches.push_back(best_approach_id);
            cur_solutions = best_solutions;
            Utils::printUIntVector(cur_approaches);
        }

    }
    // update the best solution
    solutions = cur_solutions;

    // // put it into the solution for the batch
    // printf("solutions for stripe_batch (size: %ld): \n", solutions.size());
    // for (auto solution : solutions) {
    //     printf("stripe %ld, block %ld, from: %ld, to: %ld\n", solution[0], solution[1], solution[2], solution[3]);
    // }
}

void BalancdConversion::getSolutionForStripeBatchIter(StripeBatch &stripe_batch, vector<vector<size_t> > &solutions, mt19937 random_generator) {
    ConvertibleCode &code = stripe_batch.getCode();
    ClusterSettings &settings = stripe_batch.getClusterSettings();

    vector<StripeGroup> &stripe_groups = stripe_batch.getStripeGroups();

    size_t num_stripe_groups = stripe_groups.size();

    // initialize solutions
    solutions.clear();

    // best approaches (start from all parity merging)
    vector<size_t> best_approaches(num_stripe_groups, (size_t) TransApproach::PARITY_MERGE);
    vector<vector<size_t> > best_solutions;
    size_t best_max_load = SIZE_MAX;

    for (size_t iter = 0; iter < num_stripe_groups; iter++) {
        printf("iter %ld\n", iter);
        // Step 2: construct model for each approach candidate, and find the one with minimum max-load
        // Step 1: for each stripe group, find the best approach
        for (size_t sg_id = 0; sg_id < num_stripe_groups; sg_id++) {
            // loop over all approaches
            for (size_t cand_approach_id = 0; cand_approach_id < 2; cand_approach_id++) {

                if (best_approaches[sg_id] == cand_approach_id) {
                    continue;
                }

                vector<size_t> cand_approaches = best_approaches;

                // replace sg with a new candidate approach
                cand_approaches[sg_id] = cand_approach_id;

                // construct recv bipartite graph
                RecvBipartite cur_recv_bipartite;
                cur_recv_bipartite.constructStripeBatchWithApproaches(stripe_batch, cand_approaches);

                // find edges
                vector<size_t> cand_sol_edges;
                cur_recv_bipartite.findEdgesWithApproachesGreedy(stripe_batch, cand_sol_edges, random_generator);

                // construct partial solutions from recv graph
                vector<vector<size_t> > cand_partial_solutions, cand_solutions;
                cur_recv_bipartite.constructPartialSolutionFromEdges(stripe_batch, cand_sol_edges, cand_partial_solutions);
                cur_recv_bipartite.updatePartialSolutionFromRecvGraph(stripe_batch, cand_partial_solutions, cand_solutions);

                vector<size_t> send_load_dist, recv_load_dist;
                Utils::getLoadDist(code, settings, cand_solutions, send_load_dist, recv_load_dist);

                // size_t min_send_load = *min_element(send_load_dist.begin(), send_load_dist.end());
                size_t max_send_load = *max_element(send_load_dist.begin(), send_load_dist.end());
                // size_t min_recv_load = *min_element(recv_load_dist.begin(), recv_load_dist.end());
                size_t max_recv_load = *max_element(recv_load_dist.begin(), recv_load_dist.end());
                size_t cand_max_load = max(max_send_load, max_recv_load);

                // check if the current load out-performs previous best load
                if (cand_max_load < best_max_load) {
                    printf("found better approaches, previous_load: %ld, best_max_load: %ld, approaches:\n", best_max_load, cand_max_load);
                    Utils::printUIntVector(best_approaches);

                    best_max_load = cand_max_load;
                    best_approaches = cand_approaches;
                    best_solutions = cand_solutions;
                }
            }
        }

        // update solutions
        solutions = best_solutions;

    }

    // // put it into the solution for the batch
    // printf("solutions for stripe_batch (size: %ld): \n", solutions.size());
    // for (auto solution : solutions) {
    //     printf("stripe %ld, block %ld, from: %ld, to: %ld\n", solution[0], solution[1], solution[2], solution[3]);
    // }
}