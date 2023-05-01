#include "Utils.hh"

Utils::Utils(/* args */)
{
    std::random_device rand_dev;
    std::mt19937 generator(rand_dev());
}

Utils::~Utils()
{
}

mt19937 Utils::createRandomGenerator()
{
    std::random_device rand_dev;
    std::mt19937 generator(rand_dev());

    return generator;
}

uint64_t Utils::calCombSize(uint32_t n, uint32_t k)
{
    if (k > n)
    {
        return 0;
    }
    uint64_t r = 1;
    for (uint32_t d = 1; d <= k; ++d)
    {
        r *= n--;
        r /= d;
    }
    return r;
}

uint32_t *Utils::genAllCombs(uint32_t n, uint32_t k)
{
    uint64_t comb_size = calCombSize(n, k);

    // memory consumption
    double mem_consump = 1.0 * comb_size * k * sizeof(uint32_t) / pow(2, 30);

    uint32_t *combs = (uint32_t *)malloc(comb_size * k * sizeof(uint32_t));
    if (combs == NULL)
    {
        printf("memory insufficient for generating Comb(%u, %u) = %lu\n", n, k, comb_size);
        return 0;
    }

    // print integers and permute
    u16string bitmask(k, 1);
    bitmask.resize(n, 0);
    uint64_t comb_id = 0;
    do
    {
        for (uint32_t i = 0, select_id = 0; i < n; i++) // [0..N-1] integers
        {
            if (bitmask[i])
            {
                combs[comb_id * k + select_id] = i;
                select_id++;
            }
        }
        comb_id++;
        if (comb_size % comb_id == (uint64_t)pow(10, 5))
        {
            printf("finished generation of (%ld / %ld) combinations\n", comb_id, comb_size);
        }
    } while (std::prev_permutation(bitmask.begin(), bitmask.end()));

    printf("Generated Comb(%u, %u) = %lu, required memory space (GiBytes): %f\n", n, k, comb_size, mem_consump);

    return combs;
}

u32string Utils::getCombFromPosition(uint32_t n, uint32_t k, uint64_t comb_id)
{
    u32string result(k, 0);
    u16string bitmask(n, 0);
    while (n > 0)
    {
        uint64_t y = 0;
        if (n > k && k >= 0)
        {
            y = calCombSize(n - 1, k);
        }
        if (comb_id >= y)
        {
            bitmask[n] = 1;
            comb_id -= y;
            k -= 1;
            // record in results
            result[k] = n - 1;
        }
        else
        {
            bitmask[n] = 0;
        }
        n--;
    }

    return result;
}

void Utils::getNextComb(uint32_t n, uint32_t k, u32string &cur_comb)
{
    // update sg_stripe_ids for the next combination
    uint32_t msb = k - 1, lsb = k - 1; // most (least) significant bit
    cur_comb[lsb]++;
    // handle carrying (for least significant bit)
    if (cur_comb[lsb] == n)
    {
        for (uint8_t idx = k - 1; idx > 0; idx--)
        {
            if (cur_comb[idx] == (n - (k - 1 - idx)))
            {
                msb = idx - 1;
                cur_comb[msb]++;
            }
        }
        for (uint8_t idx = msb + 1; idx <= lsb; idx++)
        {
            cur_comb[idx] = cur_comb[idx - 1] + 1;
        }
    }
}

size_t Utils::randomUInt(size_t l, size_t r, mt19937 &random_generator)
{
    std::uniform_int_distribution<size_t> distr(l, r);
    return distr(random_generator);
}

void Utils::printUIntVector(vector<size_t> &vec)
{
    for (auto item : vec)
    {
        printf("%ld ", item);
    }
    printf("\n");
}

void Utils::printUIntArray(size_t *arr, size_t len)
{
    for (size_t idx = 0; idx < len; idx++)
    {
        printf("%ld ", arr[idx]);
    }
    printf("\n");
}

void Utils::printUIntMatrix(size_t **matrix, size_t r, size_t c)
{
    for (size_t rid = 0; rid < r; rid++)
    {
        for (size_t cid = 0; cid < c; cid++)
        {
            printf("%ld ", matrix[rid][cid]);
        }
        printf("\n");
    }
    printf("\n");
}

size_t **Utils::initUIntMatrix(size_t r, size_t c)
{
    size_t **matrix = (size_t **)malloc(r * sizeof(size_t *));
    for (size_t rid = 0; rid < r; rid++)
    {
        matrix[rid] = (size_t *)calloc(c, sizeof(size_t));
    }

    return matrix;
}

void Utils::destroyUIntMatrix(size_t **matrix, size_t r, size_t c)
{
    if (matrix == NULL)
    {
        return;
    }

    for (size_t rid = 0; rid < r; rid++)
    {
        free(matrix[rid]);
    }

    free(matrix);
}

vector<size_t> Utils::argsortIntVector(vector<int> &vec)
{
    const size_t vec_len(vec.size());
    std::vector<size_t> vec_index(vec_len, 0);
    for (size_t i = 0; i < vec_len; i++)
    {
        vec_index[i] = i;
    }

    std::sort(vec_index.begin(), vec_index.end(),
              [&vec](size_t pos1, size_t pos2)
              { return (vec[pos1] < vec[pos2]); });

    return vec_index;
}

vector<size_t> Utils::argsortUIntVector(vector<size_t> &vec)
{
    const size_t vec_len(vec.size());
    std::vector<size_t> vec_index(vec_len, 0);
    for (size_t i = 0; i < vec_len; i++)
    {
        vec_index[i] = i;
    }

    std::sort(vec_index.begin(), vec_index.end(),
              [&vec](size_t pos1, size_t pos2)
              { return (vec[pos1] < vec[pos2]); });

    return vec_index;
}

void Utils::makeCombUtil(vector<vector<size_t>> &ans, vector<size_t> &tmp, size_t n, size_t left, size_t k)
{
    // Pushing this vector to a vector of vector
    if (k == 0)
    {
        ans.push_back(tmp);
        return;
    }

    // i iterates from left to n. At the beginning, left is 0
    for (size_t i = left; i < n; i++)
    {
        tmp.push_back(i);
        makeCombUtil(ans, tmp, n, i + 1, k - 1);

        // Popping out last inserted element from the vector
        tmp.pop_back();
    }
}

vector<vector<size_t>> Utils::getCombinations(size_t n, size_t k)
{
    vector<vector<size_t>> ans;
    vector<size_t> tmp;
    makeCombUtil(ans, tmp, n, 0, k);
    return ans;
}

void Utils::makePermUtil(vector<vector<size_t>> &ans, vector<size_t> tmp, size_t left, size_t right, size_t k)
{

    if (left == right)
    {
        ans.push_back(tmp);
        return;
    }

    for (size_t item = 0; item < k; item++)
    {
        tmp.push_back(item);
        makePermUtil(ans, tmp, left + 1, right, k);
        tmp.pop_back();
    }
}

vector<vector<size_t>> Utils::getPermutation(size_t n, size_t k)
{
    vector<vector<size_t>> ans;
    vector<size_t> tmp;
    makePermUtil(ans, tmp, 0, n, k);
    return ans;
}

// void Utils::getLoadDist(ConvertibleCode &code, ClusterSettings &settings, vector<vector<size_t>> &solutions, vector<size_t> &send_load_dist, vector<size_t> &recv_load_dist)
// {

//     // format for each solution: <stripe_id, block_id, from_node, to_node>

//     // initialize send_load_dist and recv_load_dist
//     send_load_dist.resize(settings.num_nodes);
//     for (auto &item : send_load_dist)
//     {
//         item = 0;
//     }
//     recv_load_dist.resize(settings.num_nodes);
//     for (auto &item : recv_load_dist)
//     {
//         item = 0;
//     }

//     // record loads for each node_id
//     for (auto &solution : solutions)
//     {
//         size_t from_node_id = solution[2];
//         size_t to_node_id = solution[3];

//         send_load_dist[from_node_id] += 1;
//         recv_load_dist[to_node_id] += 1;
//     }

//     return;
// }

vector<size_t> Utils::dotAddUIntVectors(vector<size_t> &v1, vector<size_t> &v2)
{
    if (v1.size() != v2.size())
    {
        printf("error: v1 and v2 size mismatch!\n");
    }

    vector<size_t> *lv, *rv; // lv.size >= rv.size
    if (v1.size() < v2.size())
    {
        lv = &v2;
        rv = &v1;
    }
    else
    {
        lv = &v1;
        rv = &v2;
    }

    vector<size_t> result = *lv;
    for (size_t idx = 0; idx < rv->size(); idx++)
    {
        result[idx] += (*rv)[idx];
    }

    return result;
}