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

    if (argc != 7) {
        printf("usage: ./Simulator k_i m_i k_f m_f N M placement_file");
        return -1;
    }

    int k_i = atoi(argv[1]);
    int m_i = atoi(argv[2]);
    int k_f = atoi(argv[3]);
    int m_f = atoi(argv[4]);
    int N = atoi(argv[5]);
    int M = atoi(argv[6]);

    StripeGenerator stripe_generator;

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

    vector<Stripe> stripes = stripe_generator.generateStripes(code, settings);

    printf("stripes:\n");

    for (size_t i = 0; i < stripes.size(); i++) {
        stripes[i].print();
    }

    // put all the stripes into a batch
    vector<Stripe> stripe_group_list;
    vector<StripeGroup> stripe_batch_list;

    // divide stripes sequentially into stripe groups
    for (size_t stripe_id = 0, stripe_group_id = 0; stripe_id < stripes.size(); stripe_id++) {
        Stripe &stripe = stripes[stripe_id];
        // add the stripe into stripe group
        stripe_group_list.push_back(stripe);

        if (stripe_group_list.size() == (size_t) code.lambda_i) {
            StripeGroup stripe_group(code, settings, stripe_group_id, stripe_group_list);
            stripe_batch_list.push_back(stripe_group);
            stripe_group_id++;
            stripe_group_list.clear();
        }

    }    

    StripeBatch stripe_batch(code, settings, 0);
    stripe_batch.getStripeGroups() = stripe_batch_list;

    stripe_batch.print();

    // construct recv bipartite graph
    RecvBipartite recv_bipartite;
    recv_bipartite.addStripeBatch(stripe_batch);

    recv_bipartite.print_meta();
    recv_bipartite.print();

    int recv_max_flow = -1;

    if (ENABLE_RE_ENCODING) {
        recv_max_flow = recv_bipartite.findMaxflowByFordFulkersonForRecvGraph(1, code.k_f);
    } else {
        recv_max_flow = recv_bipartite.findMaxflowByFordFulkersonForRecvGraph(1, code.k_i);
    }


    printf("maximum flow of recv graph: %d\n", recv_max_flow);

    vector<int> load_dist(settings.M, 0);
    recv_bipartite.getLoadDist(code, settings, load_dist);
    int bw_bc = 0;
    for (auto item : load_dist) {
        bw_bc += item;
    }

    printf("Balanced Conversion bandwidth: %d, load_dist:\n", bw_bc);
    Utils::print_int_vector(load_dist);


    // stripe-merge-g
    StripeMergeG stripe_merge_g;
    vector<vector<int>> smg_solutions;
    vector<int> smg_load_dist;
    stripe_merge_g.getSolutionForStripeBatch(stripe_batch, smg_solutions);
    stripe_merge_g.getLoadDist(code, settings, smg_solutions, smg_load_dist);

    int bw_smg = 0;
    for (auto item : smg_load_dist) {
        bw_smg += item;
    }
    printf("StripeMerge-G bandwidth: %d load_dist:\n", bw_smg);
    Utils::print_int_vector(smg_load_dist);


    return 0;
}