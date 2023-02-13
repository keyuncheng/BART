#include "model/Stripe.hh"
#include "model/ConvertibleCode.hh"
#include "model/ClusterSettings.hh"
#include "util/StripeGenerator.hh"
#include "model/StripeBatch.hh"
#include "model/StripeGroup.hh"
#include "model/RecvBipartite.hh"
#include "util/Utils.hh"
#include "model/StripeMergeG.hh"

int main(int argc, char *argv[]) {

    if (argc != 9) {
        printf("usage: ./Simulator k_i m_i k_f m_f N M placement_file approach[BT/SM]");
        return -1;
    }

    int k_i = atoi(argv[1]);
    int m_i = atoi(argv[2]);
    int k_f = atoi(argv[3]);
    int m_f = atoi(argv[4]);
    int N = atoi(argv[5]);
    int M = atoi(argv[6]);
    string placement_file = argv[7];
    string approach = argv[8];

    // random generator
    mt19937 random_generator = Utils::createRandomGenerator();

    // initialize code
    ConvertibleCode code(k_i, m_i, k_f, m_f);
    ClusterSettings settings;
    settings.N = N;
    settings.M = M;

    // check the number of stripes are valid
    if (Utils::isParamValid(code, settings) == false) {
        printf("invalid parameters\n");
        return -1;
    }

    // stripe generator
    StripeGenerator stripe_generator;

    // load stripes from placement file
    vector<Stripe> stripes;
    stripe_generator.loadStripes(code, settings, stripes, placement_file);

    // printf("stripes:\n");
    // for (size_t i = 0; i < stripes.size(); i++) {
    //     stripes[i].print();
    // }

    if (approach == "SM") {
        // stripe-merge-g

        // Step 1: enumerate all possible stripe groups, and sort by cost; then filter out non-overlapped stripe groups with minimum cost
        StripeBatch stripe_batch(code, settings, 0);
        stripe_batch.constructByCost(stripes);
        stripe_batch.print();

        // Step 2: generate transition solutions from given stripe groups
        StripeMergeG stripe_merge_g;
        vector<vector<int> > smg_solutions;
        vector<int> send_load_dist, recv_load_dist;
        stripe_merge_g.getSolutionForStripeBatch(stripe_batch, smg_solutions, random_generator);

        // get load distribution
        stripe_merge_g.getLoadDist(code, settings, smg_solutions, send_load_dist, recv_load_dist);

        // get bandwidth
        int bw_smg = 0;
        for (auto item : send_load_dist) {
            bw_smg += item;
        }

        printf("StripeMerge-G send load distribution:, maximum_load: %d, minimum_load: %d\n",
            *max_element(send_load_dist.begin(), send_load_dist.end()), *min_element(send_load_dist.begin(), send_load_dist.end()));
        Utils::printIntVector(send_load_dist);
        printf("StripeMerge-G recv load distribution:, maximum_load: %d, minimum_load: %d\n",
            *max_element(recv_load_dist.begin(), recv_load_dist.end()), *min_element(recv_load_dist.begin(), recv_load_dist.end()));
        Utils::printIntVector(recv_load_dist);
        printf("StripeMerge-G bandwidth: %d\n", bw_smg);
    }
    // construct a stripe batch

    // StripeBatch stripe_batch_seq(code, settings, 0);
    // stripe_batch_seq.constructInSequence(stripes);
    // stripe_batch_seq.print();

    // StripeBatch stripe_batch_rand(code, settings, 0);
    // stripe_batch_rand.constructByRandomPick(stripes, random_generator);
    // stripe_batch_rand.print();





    // // construct recv bipartite graph
    // RecvBipartite recv_bipartite;
    // recv_bipartite.addStripeBatch(stripe_batch);

    // recv_bipartite.print_meta();
    // recv_bipartite.print();

    // int recv_max_flow = -1;

    // if (ENABLE_RE_ENCODING) {
    //     recv_max_flow = recv_bipartite.findMaxflowByFordFulkersonForRecvGraph(1, code.k_f);
    // } else {
    //     recv_max_flow = recv_bipartite.findMaxflowByFordFulkersonForRecvGraph(1, code.k_i);
    // }


    // printf("maximum flow of recv graph: %d\n", recv_max_flow);

    // vector<int> load_dist(settings.M, 0);
    // recv_bipartite.getLoadDist(code, settings, load_dist);
    // int bw_bc = 0;
    // for (auto item : load_dist) {
    //     bw_bc += item;
    // }

    // printf("Balanced Conversion bandwidth: %d, load_dist:\n", bw_bc);
    // Utils::printIntVector(load_dist);


    return 0;
}