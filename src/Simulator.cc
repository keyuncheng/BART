#include "model/Stripe.hh"
#include "model/ConvertibleCode.hh"
#include "model/ClusterSettings.hh"
#include "util/StripeGenerator.hh"
#include "model/StripeBatch.hh"
#include "model/StripeGroup.hh"
#include "model/RecvBipartite.hh"
#include "util/Utils.hh"

int main(int argc, char *argv[]) {

    if (argc != 7) {
        printf("usage: ./Simulator k_i m_i k_f m_f N M");
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

    vector<Stripe> stripes = stripe_generator.GenerateStripes(code, settings);

    printf("stripes:\n");

    for (auto i = 0; i < stripes.size(); i++) {
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

    vector<vector<int>> recv_graph_paths;
    // copy the vertices and metadata to solution

    int recv_max_flow = RecvBipartite::findMaxflowByFordFulkersonForRecvGraph(recv_bipartite, recv_graph_paths, 1, code.k_f);

    if (recv_bipartite.buildMaxFlowSolutionFromPaths(recv_graph_paths) == false) {
        printf("invalid max flow solution\n");
        return -1;
    }

    printf("maximum flow of recv graph: %d\n", recv_max_flow);
    printf("edges of max_flow:\n");
    recv_bipartite.print_edges(recv_bipartite.edges_max_flow_map);

    return 0;
}