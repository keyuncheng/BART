#include "StripeBatch.hh"

StripeBatch::StripeBatch(uint8_t _id, ConvertibleCode &_code, ClusterSettings &_settings, mt19937 &_random_generator) : id(_id), code(_code), settings(_settings), random_generator(_random_generator)
{

    // init pre-transition stripes
    pre_stripes.clear();
    pre_stripes.assign(settings.num_stripes, Stripe());
    for (uint32_t stripe_id = 0; stripe_id < settings.num_stripes; stripe_id++)
    {
        pre_stripes[stripe_id].id = stripe_id;
        pre_stripes[stripe_id].indices.assign(code.n_i, INVALID_NODE_ID);
    }

    // init post-transition stripes
    uint32_t num_post_stripes = settings.num_stripes / code.lambda_i;
    post_stripes.clear();
    post_stripes.assign(num_post_stripes, Stripe());
    for (uint32_t post_stripe_id = 0; post_stripe_id < num_post_stripes; post_stripe_id++)
    {
        post_stripes[post_stripe_id].id = post_stripe_id;
        post_stripes[post_stripe_id].indices.assign(code.n_f, INVALID_NODE_ID);
    }
}

StripeBatch::~StripeBatch()
{
}

void StripeBatch::print()
{
    printf("StripeBatch %u:\n", id);
    for (auto &item : selected_sgs)
    {
        item.second.print();
    }
}

void StripeBatch::constructSGInSequence()
{
    uint32_t num_sgs = settings.num_stripes / code.lambda_i;

    selected_sgs.clear();

    // initialize sg_stripe_ids
    for (uint32_t sg_id = 0; sg_id < num_sgs; sg_id++)
    {
        u32string sg_stripe_ids(code.lambda_i, 0);
        vector<Stripe *> selected_pre_stripes(code.lambda_i, NULL);
        for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
        {
            uint32_t sb_stripe_id = sg_id * code.lambda_i + stripe_id;
            sg_stripe_ids[stripe_id] = sb_stripe_id;
            selected_pre_stripes[stripe_id] = &pre_stripes[sb_stripe_id];
        }
        selected_sgs.insert(pair<uint32_t, StripeGroup>(sg_id, StripeGroup(sg_id, code, settings, selected_pre_stripes, &post_stripes[sg_id])));
    }
}

void StripeBatch::constructSGByRandomPick()
{
    uint32_t num_sgs = settings.num_stripes / code.lambda_i;

    selected_sgs.clear();

    // shuffle indices of stripes
    vector<uint32_t> shuff_sb_stripe_idxs(pre_stripes.size(), 0);
    for (uint32_t stripe_id = 0; stripe_id < pre_stripes.size(); stripe_id++)
    {
        shuff_sb_stripe_idxs[stripe_id] = stripe_id;
    }

    shuffle(shuff_sb_stripe_idxs.begin(), shuff_sb_stripe_idxs.end(), random_generator);

    // initialize sg_stripe_ids
    for (uint32_t sg_id = 0; sg_id < num_sgs; sg_id++)
    {
        u32string sg_stripe_ids(code.lambda_i, 0);
        vector<Stripe *> selected_pre_stripes(code.lambda_i, NULL);
        for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
        {
            // get shuffled stripe index
            uint32_t sb_stripe_id = shuff_sb_stripe_idxs[sg_id * code.lambda_i + stripe_id];
            sg_stripe_ids[stripe_id] = sb_stripe_id;
            selected_pre_stripes[stripe_id] = &pre_stripes[sb_stripe_id];
        }
        selected_sgs.insert(pair<uint32_t, StripeGroup>(sg_id, StripeGroup(sg_id, code, settings, selected_pre_stripes, &post_stripes[sg_id])));
    }
}

void StripeBatch::constructSGByBWBF(string approach)
{
    uint32_t num_sgs = settings.num_stripes / code.lambda_i;

    selected_sgs.clear();

    // enumerate all possible stripe groups
    uint64_t num_combs = Utils::calCombSize(settings.num_stripes, code.lambda_i);

    uint64_t num_cand_sgs = num_combs;
    printf("candidates stripe groups: %ld\n", num_cand_sgs);

    /**
     * @brief bandwidth table for all stripe groups
     *  bandwidth 0: sg_0; sg_1; ...
     *  bandwidth 1: sg_2; sg_3; ...
     *  ...
     *  bandwidth n_f: ...
     */

    uint8_t max_bw = code.n_f + code.k_f;           // allowed maximum bandwidth
    vector<vector<u32string>> bw_sgs_table(max_bw); // record sg_stripe_ids for each candidate stripe group

    // current stripe ids
    u32string sg_stripe_ids(code.lambda_i, 0);
    for (uint32_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
    {
        sg_stripe_ids[stripe_id] = stripe_id;
    }

    // calculate transition bandwidth for each stripe group
    for (uint64_t cand_sg_id = 0; cand_sg_id < num_cand_sgs; cand_sg_id++)
    {
        // // get stripe ids
        // u32string sg_stripe_ids = Utils::getCombFromPosition(settings.num_stripes, code.lambda_i, cand_sg_id);

        vector<Stripe *> selected_pre_stripes(code.lambda_i, NULL);
        for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
        {
            selected_pre_stripes[stripe_id] = &pre_stripes[sg_stripe_ids[stripe_id]];
        }

        StripeGroup stripe_group(0, code, settings, selected_pre_stripes, NULL);
        u16string sg_enc_nodes(code.m_f, INVALID_NODE_ID);
        uint8_t min_bw = stripe_group.getTransBW(approach, sg_enc_nodes);
        bw_sgs_table[min_bw].push_back(sg_stripe_ids);

        // printf("candidate stripe group: %lu, minimum bandwidth: %u\n", cand_sg_id, min_bw);
        // Utils::printVector(sg_stripe_ids);

        // get next combination
        Utils::getNextComb(settings.num_stripes, code.lambda_i, sg_stripe_ids);

        // logging
        if (cand_sg_id % (uint64_t)pow(10, 7) == 0)
        {
            printf("stripe groups (%lu / %lu) bandwidth calculated\n", cand_sg_id, num_cand_sgs);
        }
    }

    // printf("bw_sgs_table:\n");
    // for (uint8_t bw = 0; bw < max_bw; bw++)
    // {
    //     printf("bandwidth: %u, %lu candidate stripe groups\n", bw, bw_sgs_table[bw].size());
    // }

    vector<uint32_t> bw_num_selected_sgs(max_bw, 0);           // record bw for selected stripes
    vector<bool> stripe_selected(settings.num_stripes, false); // mark if stripe is selected

    uint32_t sg_id = 0;
    for (uint8_t bw = 0; bw < max_bw && sg_id < num_sgs; bw++)
    {
        for (uint64_t idx = 0; idx < bw_sgs_table[bw].size(); idx++)
        {
            u32string &sg_stripe_ids = bw_sgs_table[bw][idx];
            // auto cand_sg_id = bw_sgs_table[bw][idx];
            // u32string sg_stripe_ids = Utils::getCombFromPosition(settings.num_stripes, code.lambda_i, cand_sg_id);

            // check whether the stripe group is valid
            bool is_cand_sg_valid = true;
            for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
            {
                is_cand_sg_valid = is_cand_sg_valid && !stripe_selected[sg_stripe_ids[stripe_id]];
            }

            if (is_cand_sg_valid == true)
            {
                // add the valid stripe group
                vector<Stripe *> selected_pre_stripes(code.lambda_i, NULL);
                for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
                {
                    selected_pre_stripes[stripe_id] = &pre_stripes[sg_stripe_ids[stripe_id]];
                }
                selected_sgs.insert(pair<uint32_t, StripeGroup>(sg_id, StripeGroup(sg_id, code, settings, selected_pre_stripes, &post_stripes[sg_id])));

                // mark the stripes as selected
                sg_id++;
                for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
                {
                    stripe_selected[sg_stripe_ids[stripe_id]] = true;
                }

                // update stats
                bw_num_selected_sgs[bw]++;

                // printf("(%u / %u) stripe groups selected, bandwidth: %u, stripes: ", num_selected_sgs, num_sgs, bw);
                // Utils::printVector(sg_stripe_ids);
            }
        }
    }

    uint32_t total_bw_selected_sgs = 0;
    for (uint8_t bw = 0; bw < max_bw; bw++)
    {
        total_bw_selected_sgs += bw_num_selected_sgs[bw] * bw;
    }

    // printf("selected %u stripe groups, total bandwidth: %u\n", num_sgs, total_bw_selected_sgs);
    // printf("bw_num_selected_sgs:\n");
    // for (uint8_t bw = 0; bw < max_bw; bw++)
    // {
    //     if (bw_num_selected_sgs[bw] > 0)
    //     {
    //         printf("bandwidth = %u: %u stripe groups\n", bw, bw_num_selected_sgs[bw]);
    //     }
    // }
}

void StripeBatch::constructSGByBWPartial(string approach)
{
    uint32_t num_sgs = settings.num_stripes / code.lambda_i;
    uint8_t max_bw = code.n_f + code.k_f; // allowed maximum bandwidth

    selected_sgs.clear();

    // benchmarking
    double finish_time = 0.0;
    struct timeval start_time, end_time;
    gettimeofday(&start_time, nullptr);

    // current partial stripe group
    vector<u32string> cur_partial_sgs;                        // store currently selected partial stripe groups
    vector<uint64_t> cur_bw_num_partial_sgs_table(max_bw, 0); // record the bandwidth stats

    // we execute the pairing for code.lambda_i - 1 iterations; every time after pairing, we keep <num_sgs> non-overlapped partial stripe groups with ascending order of bandwidth, which will be paired with remaining stripes in the next iteration; finally, we obtain <num_sgs> stripe groups
    for (uint8_t iter = 0; iter < code.lambda_i - 1; iter++)
    {

        vector<u32string> updated_partial_sgs; // updated partial stripe groups (by adding pre_stripe into current partial stripe group; max_size: num_stripes * (num_stripes - 1) / 2)

        // construct candidate partial stripe groups
        if (iter == 0)
        {
            // initialize partial stripe group as all stripe groups in size = 2
            u32string partial_sg(2, INVALID_STRIPE_ID_GLOBAL);
            for (uint32_t pre_stripe_x = 0; pre_stripe_x < settings.num_stripes - 1; pre_stripe_x++)
            {
                for (uint32_t pre_stripe_y = pre_stripe_x + 1; pre_stripe_y < settings.num_stripes; pre_stripe_y++)
                {
                    partial_sg[0] = pre_stripe_x;
                    partial_sg[1] = pre_stripe_y;
                    updated_partial_sgs.push_back(partial_sg);
                }
            }
        }
        else
        {
            // match <cur_partial_sgs> with the remaining stripes

            // first, make sure that the partial stripes are correctly chosen
            vector<bool> is_pre_stripe_selected(settings.num_stripes, false); // mark if stripe is selected
            for (auto &partial_sg : cur_partial_sgs)
            {
                for (auto pre_stripe_id : partial_sg)
                {
                    is_pre_stripe_selected[pre_stripe_id] = true;
                }
            }
            uint32_t num_selected_stripes = accumulate(is_pre_stripe_selected.begin(), is_pre_stripe_selected.end(), 0);
            if (num_selected_stripes != num_sgs * (iter + 1))
            {
                printf("error: invalid partial stripe group selection!\n");
                exit(EXIT_FAILURE);
            }

            for (auto &partial_sg : cur_partial_sgs)
            {
                for (uint32_t pre_stripe_id = 0; pre_stripe_id < settings.num_stripes; pre_stripe_id++)
                {
                    if (is_pre_stripe_selected[pre_stripe_id] == true)
                    {
                        continue;
                    }

                    // construct updated_partial_sg
                    u32string updated_partial_sg = partial_sg;
                    updated_partial_sg.push_back(pre_stripe_id);
                    sort(updated_partial_sg.begin(), updated_partial_sg.end());

                    // add the updated_partial_sg
                    updated_partial_sgs.push_back(updated_partial_sg);
                }
            }
        }

        // printf("updated_partial_sgs: %lu\n", updated_partial_sgs.size());
        // for (auto &partial_sg : updated_partial_sgs)
        // {
        //     Utils::printVector(partial_sg);
        // }

        /**
         * @brief bandwidth table for all stripe groups
         *  bandwidth 0: sg_0; sg_1; ...
         *  bandwidth 1: sg_2; sg_3; ...
         *  ...
         *  bandwidth n_f: ...
         */
        vector<vector<uint64_t>> bw_partial_sgs_table(max_bw); // record partial_sg_id for each candidate stripe group
        u16string sg_enc_nodes(code.m_f, INVALID_NODE_ID);

        for (uint64_t partial_sg_id = 0; partial_sg_id < updated_partial_sgs.size(); partial_sg_id++)
        {
            u32string &partial_sg = updated_partial_sgs[partial_sg_id];
            vector<Stripe *> updated_partial_pre_stripes;
            for (auto pre_stripe_id : partial_sg)
            {
                updated_partial_pre_stripes.push_back(&pre_stripes[pre_stripe_id]);
            }

            // calculate bandwidth
            StripeGroup partial_stripe_group(partial_sg_id, code, settings, updated_partial_pre_stripes, NULL);

            uint8_t min_bw = partial_stripe_group.getTransBW(approach, sg_enc_nodes);

            // update bw table
            bw_partial_sgs_table[min_bw].push_back(partial_sg_id);
        }

        // printf("bw_partial_sgs_table:\n");
        // for (uint8_t bw = 0; bw < max_bw; bw++)
        // {
        //     if (bw_partial_sgs_table[bw].size() > 0)
        //     {
        //         printf("bandwidth = %u: %lu stripe groups\n", bw, bw_partial_sgs_table[bw].size());
        //         for (auto &partial_sg_id : bw_partial_sgs_table[bw])
        //         {
        //             Utils::printVector(updated_partial_sgs[partial_sg_id]);
        //         }
        //     }
        // }

        // reset the current selection
        cur_partial_sgs.clear();
        cur_bw_num_partial_sgs_table.assign(max_bw, 0);
        vector<bool> is_pre_stripe_selected(settings.num_stripes, false); // mark if pre_stripe is selected

        // choose partial_sgs with lowest bandwidth
        for (uint8_t bw = 0; bw < max_bw; bw++)
        {
            for (auto partial_sg_id : bw_partial_sgs_table[bw])
            {
                u32string &partial_sg = updated_partial_sgs[partial_sg_id];

                // check whether the stripe group is valid
                bool is_partial_sg_valid = true;
                for (auto pre_stripe_id : partial_sg)
                {
                    // is_partial_sg_valid = is_partial_sg_valid && !stripe_selected[sg_stripe_ids[stripe_id]];
                    if (is_pre_stripe_selected[pre_stripe_id] == true)
                    {
                        is_partial_sg_valid = false;
                        break;
                    }
                }

                if (is_partial_sg_valid == false)
                {
                    continue;
                }

                // add the partial stripe group
                cur_partial_sgs.push_back(partial_sg);
                cur_bw_num_partial_sgs_table[bw]++;
                for (auto pre_stripe_id : partial_sg)
                { // mark the stripes as selected
                    is_pre_stripe_selected[pre_stripe_id] = true;
                }

                if (cur_partial_sgs.size() >= num_sgs)
                { // only select <num_sgs> partial stripe groups
                    break;
                }
            }

            if (cur_partial_sgs.size() >= num_sgs)
            { // only select <num_sgs> partial stripe groups
                break;
            }
        }

        uint32_t total_bw_selected_partial_sgs = 0;
        for (uint8_t bw = 0; bw < max_bw; bw++)
        {
            total_bw_selected_partial_sgs += cur_bw_num_partial_sgs_table[bw] * bw;
        }

        printf("iter %u: selected (%lu / %lu) partial stripe groups, bandwidth: %u\n", iter, cur_partial_sgs.size(), updated_partial_sgs.size(), total_bw_selected_partial_sgs);

        // printf("cur_partial_sgs:\n");
        // for (auto &partial_sg : cur_partial_sgs)
        // {
        //     Utils::printVector(partial_sg);
        // }

        printf("cur_bw_num_partial_sgs_table:\n");
        for (uint8_t bw = 0; bw < max_bw; bw++)
        {
            if (cur_bw_num_partial_sgs_table[bw] > 0)
            {
                printf("bandwidth = %u: %lu stripe groups\n", bw, cur_bw_num_partial_sgs_table[bw]);
            }
        }
    }

    // choose stripe groups from cur_partial_sgs in the last iteration
    vector<bool> stripe_selected(settings.num_stripes, false); // mark if stripe is selected
    uint32_t selected_sg_id = 0;
    for (auto &sg_stripe_ids : cur_partial_sgs)
    {
        vector<Stripe *> selected_pre_stripes;
        for (auto pre_stripe_id : sg_stripe_ids)
        {
            selected_pre_stripes.push_back(&pre_stripes[pre_stripe_id]);
            stripe_selected[pre_stripe_id] = true;
        }
        selected_sgs.insert(pair<uint32_t, StripeGroup>(selected_sg_id, StripeGroup(selected_sg_id, code, settings, selected_pre_stripes, &post_stripes[selected_sg_id])));
        selected_sg_id++;
    }

    uint32_t num_selected_stripes = accumulate(stripe_selected.begin(), stripe_selected.end(), 0);
    if (num_selected_stripes != settings.num_stripes)
    {
        printf("error: invalid stripe group selection!\n");
        exit(EXIT_FAILURE);
    }

    gettimeofday(&end_time, nullptr);
    finish_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                  (end_time.tv_usec - start_time.tv_usec) / 1000;
    printf("finished constructing %lu stripe groups, time: %f ms\n", selected_sgs.size(), finish_time);
}

void StripeBatch::storeSGMetadata(string sg_meta_filename)
{
    if (selected_sgs.size() == 0)
    {
        printf("invalid number of stripe groups\n");
        return;
    }

    ofstream of;

    of.open(sg_meta_filename, ios::out);

    for (auto &item : selected_sgs)
    {
        // of << item.first << " ";
        StripeGroup &stripe_group = item.second;

        // store pre-stripe ids
        for (auto &stripe : stripe_group.pre_stripes)
        {
            of << stripe->id << " ";
        }
        // store parity computation method
        of << stripe_group.parity_comp_method << " ";

        // store parity computation nodes
        for (auto parity_compute_node : stripe_group.parity_comp_nodes)
        {
            of << parity_compute_node << " ";
        }
        of << endl;
    }

    of.close();

    printf("finished storing %lu stripe groups in %s\n", selected_sgs.size(), sg_meta_filename.c_str());
}

bool StripeBatch::loadSGMetadata(string sg_meta_filename)
{
    selected_sgs.clear();

    ifstream ifs(sg_meta_filename.c_str());

    if (ifs.fail())
    {
        printf("invalid sg_meta file: %s\n", sg_meta_filename.c_str());
        return false;
    }

    string line;
    uint32_t sg_id = 0;
    while (getline(ifs, line))
    {

        // get indices
        istringstream iss(line);

        unsigned int val;

        vector<Stripe *> selected_pre_stripes(code.lambda_i, NULL);

        // load pre-stripe ids
        for (uint8_t pre_stripe_id = 0; pre_stripe_id < code.lambda_i; pre_stripe_id++)
        {
            iss >> val;
            uint32_t pre_stripe_id_global = val;
            selected_pre_stripes[pre_stripe_id] = &pre_stripes[pre_stripe_id_global];
        }

        selected_sgs.insert(pair<uint32_t, StripeGroup>(sg_id, StripeGroup(sg_id, code, settings, selected_pre_stripes, &post_stripes[sg_id])));

        StripeGroup &stripe_group = selected_sgs.find(sg_id)->second;

        // load parity computation method
        iss >> val;
        stripe_group.parity_comp_method = (EncodeMethod)val;

        // load parity computation nodes
        stripe_group.parity_comp_nodes.assign(code.m_f, INVALID_NODE_ID);
        for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
        {
            iss >> val;
            stripe_group.parity_comp_nodes[parity_id] = val;
        }

        sg_id++;
    }

    ifs.close();

    printf("finished loading %lu stripes group metadata from %s\n", selected_sgs.size(), sg_meta_filename.c_str());

    return true;
}