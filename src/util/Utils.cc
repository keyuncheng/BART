#include "Utils.hh"

Utils::Utils(/* args */)
{
    std::random_device rand_dev;
    std::mt19937 generator(rand_dev());
}

Utils::~Utils()
{
}

bool Utils::isParamValid(const ConvertibleCode &code, const ClusterSettings &settings) {

    if (code.k_i == 0 || code.m_i == 0 || code.k_f == 0 || code.m_f == 0) {
        return false;
    }

    if (settings.M == 0 || settings.N == 0) {
        return false;
    }

    size_t num_stripes = settings.N;
    if (num_stripes % code.lambda_i != 0) {
        return false;
    }

    return true;
}

mt19937 Utils::createRandomGenerator() {
    std::random_device rand_dev;
    std::mt19937 generator(rand_dev());

    return generator;
}

size_t Utils::randomUInt(size_t l, size_t r, mt19937 &random_generator) {
    std::uniform_int_distribution<size_t> distr(l, r);
    return distr(random_generator);
}

void Utils::printUIntVector(vector<size_t> &vec) {
    for (auto item : vec) {
        printf("%ld ", item);
    }
    printf("\n");
}

void Utils::printUIntArray(size_t *arr, size_t len) {
    for (size_t idx = 0; idx < len; idx++) {
        printf("%ld ", arr[idx]);
    }
    printf("\n");
}

void Utils::printUIntMatrix(size_t **matrix, size_t r, size_t c) {
    for (size_t rid = 0; rid < r; rid++) {
        for (size_t cid = 0; cid < c; cid++) {
            printf("%ld ", matrix[rid][cid]);
        }
        printf("\n");
    }
    printf("\n");
}

size_t **Utils::initUIntMatrix(size_t r, size_t c) {
    size_t **matrix = (size_t **) malloc(r * sizeof(size_t *));
    for (size_t rid = 0; rid < r; rid++) {
        matrix[rid] = (size_t *) calloc(c, sizeof(size_t));
    }

    return matrix;
}

void Utils::destroyUIntMatrix(size_t **matrix, size_t r, size_t c) {
    if (matrix == NULL) {
        return;
    }

    for (size_t rid = 0; rid < r; rid++) {
        free(matrix[rid]);
    }

    free(matrix);
}

vector<size_t> Utils::argsortIntVector(vector<int> &vec) {
    const size_t vec_len(vec.size());
    std::vector<size_t> vec_index(vec_len, 0);
    for (size_t i = 0; i < vec_len; i++) {
        vec_index[i] = i;
    }

    std::sort(vec_index.begin(), vec_index.end(),
        [&vec](size_t pos1, size_t pos2) { return (vec[pos1] < vec[pos2]); });

    return vec_index;
}

vector<size_t> Utils::argsortUIntVector(vector<size_t> &vec) {
    const size_t vec_len(vec.size());
    std::vector<size_t> vec_index(vec_len, 0);
    for (size_t i = 0; i < vec_len; i++) {
        vec_index[i] = i;
    }

    std::sort(vec_index.begin(), vec_index.end(),
        [&vec](size_t pos1, size_t pos2) { return (vec[pos1] < vec[pos2]); });

    return vec_index;
}


void Utils::makeCombUtil(vector<vector<size_t> >& ans, vector<size_t>& tmp, size_t n, size_t left, size_t k) {
    // Pushing this vector to a vector of vector
    if (k == 0) {
        ans.push_back(tmp);
        return;
    }
 
    // i iterates from left to n. At the beginning, left is 0
    for (size_t i = left; i < n; i++) {
        tmp.push_back(i);
        makeCombUtil(ans, tmp, n, i + 1, k - 1);
 
        // Popping out last inserted element from the vector
        tmp.pop_back();
    }
}

vector<vector<size_t> > Utils::getCombinations(size_t n, size_t k) {
    vector<vector<size_t> > ans;
    vector<size_t> tmp;
    makeCombUtil(ans, tmp, n, 0, k);
    return ans;
}

void Utils::makeEnumUtil(vector<vector<size_t> >& ans, vector<size_t> tmp, size_t left, size_t right, size_t k) {
    
    if (left == right) {
        ans.push_back(tmp);
        return;
    }

    for (size_t item = 0; item < k; item++) {
        tmp.push_back(item);
        makeEnumUtil(ans, tmp, left + 1, right, k);
        tmp.pop_back();
    }
}

vector<vector<size_t> > Utils::getEnumeration(size_t n, size_t k) {
    vector<vector<size_t>> ans;
    vector<size_t> tmp;
    makeEnumUtil(ans, tmp, 0, n, k);
    return ans;
}

void Utils::getLoadDist(ConvertibleCode &code, ClusterSettings &settings, vector<vector<size_t> > &solutions, vector<size_t> &send_load_dist, vector<size_t> &recv_load_dist) {
    size_t num_nodes = settings.M;

    // format for each solution: <stripe_id, block_id, from_node, to_node>

    // initialize send_load_dist and recv_load_dist
    send_load_dist.resize(num_nodes);
    for (auto &item : send_load_dist) {
        item = 0;
    }
    recv_load_dist.resize(num_nodes);
    for (auto &item : recv_load_dist) {
        item = 0;
    }
    
    // record loads for each node_id
    for (auto &solution : solutions) {
        size_t from_node_id = solution[2];
        size_t to_node_id = solution[3];

        send_load_dist[from_node_id] += 1;
        recv_load_dist[to_node_id] += 1;
    }

    return;
}