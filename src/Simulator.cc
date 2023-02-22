#include "model/Stripe.hh"
#include "model/ConvertibleCode.hh"
#include "model/ClusterSettings.hh"
#include "util/StripeGenerator.hh"
#include "model/StripeBatch.hh"
#include "model/StripeGroup.hh"
#include "model/RecvBipartite.hh"
#include "util/Utils.hh"
#include "model/StripeMergeG.hh"
#include "model/BalancdConversion.hh"

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
    stripe_generator.loadStripes(code, settings, placement_file, stripes);

    // printf("stripes:\n");
    // for (size_t i = 0; i < stripes.size(); i++) {
    //     stripes[i].print();
    // }

    // solutions and load distribution
    vector<vector<int> > solutions;
    vector<int> send_load_dist, recv_load_dist;

    if (approach == "SM") {
        // stripe-merge-g

        // Step 1: enumerate all possible stripe groups; pick non-overlapped stripe groups in ascending order of transition costs (bandwidth)
        StripeBatch stripe_batch(0, code, settings);
        stripe_batch.constructByCost(stripes);
        stripe_batch.print();

        // Step 2: generate transition solutions from all stripe groups
        StripeMergeG stripe_merge_g;
        stripe_merge_g.getSolutionForStripeBatch(stripe_batch, solutions, random_generator);

    } else if (approach == "BT") {
        // balanced transition

        // Step 1: construct a stripe batch
        
        // possible construction techniques: sequentially construct; randomly construct; construct by cost
        StripeBatch stripe_batch(0, code, settings);

        // stripe_batch.constructInSequence(stripes);
        // stripe_batch.constructByRandomPick(stripes, random_generator);
        stripe_batch.constructByCost(stripes);
        stripe_batch.print();

        BalancdConversion balanced_conversion;
        balanced_conversion.getSolutionForStripeBatch(stripe_batch, solutions, random_generator);
    }

    // get load distribution
    Utils::getLoadDist(code, settings, solutions, send_load_dist, recv_load_dist);

    // get bandwidth
    int bw = 0;
    for (auto item : send_load_dist) {
        bw += item;
    }

    int min_send_load = *min_element(send_load_dist.begin(), send_load_dist.end());
    int max_send_load = *max_element(send_load_dist.begin(), send_load_dist.end());
    int min_recv_load = *min_element(recv_load_dist.begin(), recv_load_dist.end());
    int max_recv_load = *max_element(recv_load_dist.begin(), recv_load_dist.end());

    printf("%s send load distribution:, minimum_load: %d, maximum_load: %d\n", approach.c_str(), min_send_load, max_send_load);
    Utils::printIntVector(send_load_dist);
    printf("%s recv load distribution:, minimum_load: %d, maximum_load: %d\n", approach.c_str(), min_recv_load, max_recv_load);
    Utils::printIntVector(recv_load_dist);
    printf("%s bandwidth: %d\n", approach.c_str(), bw);



    // prev: construct with min-cost max-flow

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