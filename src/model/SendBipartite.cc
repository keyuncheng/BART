#include "SendBipartite.hh"

SendBipartite::SendBipartite(/* args */)
{
}

SendBipartite::~SendBipartite()
{
}

bool SendBipartite::addStripeBatchFromRecvGraph(StripeBatch &stripe_batch,  RecvBipartite &recv_bipartite) {
    for (auto stripe_group : stripe_batch.getStripeGroups()) {
        if (addStripeGroupFromRecvGraph(stripe_group, recv_bipartite) == false) {
            return false;
        }
    }

    return true;
}

bool SendBipartite::addStripeGroupFromRecvGraph(StripeGroup &stripe_group, RecvBipartite &recv_bipartite) {
    ConvertibleCode &code = stripe_group.getCode();
    
    bool ret_val = true;

    // if k' = alpha * k
    if (code.k_f == code.k_i * code.alpha) {
        ret_val = addStripeGroupWithParityMergingFromRecvGraph(stripe_group, recv_bipartite);
    } else {
        ret_val = false;
    }

    return ret_val;
}

bool SendBipartite::addStripeGroupWithParityMergingFromRecvGraph(StripeGroup &stripe_group, RecvBipartite &recv_bipartite) {
    ConvertibleCode &code = stripe_group.getCode();

    // if k' = alpha * k
    if (code.k_f != code.k_i * code.alpha || code.m_f > code.m_i) {
        printf("invalid parameters\n");
        return false;
    }

    ClusterSettings &cluster_settings = stripe_group.getClusterSettings();
    int num_nodes = cluster_settings.M;

    // find data blocks for corresponding stripe group
    for (auto it = recv_bipartite.left_vertices_map.begin(); it != recv_bipartite.left_vertices_map.end(); it++) {
        BlockVtx &bvtx = it->second;
        if (bvtx.stripe_group_id != stripe_group.getId()) {
            continue;
        }

        if (bvtx.block_id >= code.k_i) {
            continue;
        }

        // the receive graph should already have one to one correspondence
        
    }

}


void SendBipartite::print() {
    printf("recv bipartite graph:\n");
    printf("Left vertices:\n");
    for (auto it = left_vertices_map.begin(); it != left_vertices_map.end(); it++) {
        int lvtx_id = it->first;
        BlockVtx &lvtx = it->second;
        printf("id: %d, stripe_group_id: %d, stripe_id_global: %d, block_id: %d, out_degree: %d\n", lvtx_id, lvtx.stripe_group_id, lvtx.stripe_id_global, lvtx.block_id, lvtx.out_degree);
    }

    printf("Right vertices:\n");
    for (auto it = right_vertices_map.begin(); it != right_vertices_map.end(); it++) {
        int rvtx_id = it->first;
        NodeVtx &rvtx = it->second;
        printf("node_id: %d, id: %d, in_degree: %d\n", rvtx_id, rvtx.id, rvtx.in_degree);
    }

    printf("Edges: \n");
    for (auto it = edges_map.begin(); it != edges_map.end(); it++) {
        int edge_id = it->first;
        Edge &edge = it->second;
        printf("id: %d, lvtx(.id): %d, rvtx(.node_id): %d, weight: %d\n", edge_id, edge.lvtx->id, edge.rvtx->node_id, edge.weight);
    }
}