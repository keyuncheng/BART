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
    int num_stripes = settings.N;

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

int Utils::randomInt(int l, int r, mt19937 &random_generator) {
    std::uniform_int_distribution<int> distr(l, r);
    return distr(random_generator);
}

void Utils::printIntVector(vector<int> &vec) {
    for (auto item : vec) {
        printf("%d ", item);
    }
    printf("\n");
}

void Utils::printIntArray(int *arr, int len) {
    if (len <= 0) {
        printf("invalid input array dimensions\n");
        return;
    }
    for (int idx = 0; idx < len; idx++) {
        printf("%d ", arr[idx]);
    }
    printf("\n");
}

void Utils::printIntMatrix(int **matrix, int r, int c) {
    if (r <= 0 || c <= 0) {
        printf("invalid input matrix dimensions\n");
        return;
    }

    for (int rid = 0; rid < r; rid++) {
        for (int cid = 0; cid < c; cid++) {
            printf("%d ", matrix[rid][cid]);
        }
        printf("\n");
    }
    printf("\n");
}

int **Utils::initIntMatrix(int r, int c) {
    if (r <= 0 || c <= 0) {
        printf("invalid input matrix dimensions\n");
        return NULL;
    }

    int **matrix = (int **) malloc(r * sizeof(int *));
    for (int rid = 0; rid < r; rid++) {
        matrix[rid] = (int *) calloc(c, sizeof(int));
    }

    return matrix;
}

void Utils::destroyIntMatrix(int **matrix, int r, int c) {
    if (matrix == NULL) {
        return;
    }

    for (int rid = 0; rid < r; rid++) {
        free(matrix[rid]);
    }

    free(matrix);
}

vector<int> Utils::argsortIntVector(vector<int> &vec) {
    const int vec_len(vec.size());
    std::vector<int> vec_index(vec_len, 0);
    for (int i = 0; i < vec_len; i++) {
        vec_index[i] = i;
    }

    std::sort(vec_index.begin(), vec_index.end(),
        [&vec](int pos1, int pos2) { return (vec[pos1] < vec[pos2]); });

    return vec_index;
}


void Utils::makeCombUtil(vector<vector<int> >& ans, vector<int>& tmp, int n, int left, int k) {
    // Pushing this vector to a vector of vector
    if (k == 0) {
        ans.push_back(tmp);
        return;
    }
 
    // i iterates from left to n. At the beginning, left is 0
    for (int i = left; i < n; i++) {
        tmp.push_back(i);
        makeCombUtil(ans, tmp, n, i + 1, k - 1);
 
        // Popping out last inserted element from the vector
        tmp.pop_back();
    }
}

vector<vector<int> > Utils::getCombinations(int n, int k) {
    vector<vector<int> > ans;
    vector<int> tmp;
    makeCombUtil(ans, tmp, n, 0, k);
    return ans;
}

void Utils::makeEnumUtil(vector<vector<int> >& ans,
    vector<int> tmp, int left, int right, int k) {
    
    if (left == right) {
        ans.push_back(tmp);
        return;
    }

    for (int item = 0; item < k; item++) {
        tmp.push_back(item);
        makeEnumUtil(ans, tmp, left + 1, right, k);
        tmp.pop_back();
    }
}

vector<vector<int> > Utils::getEnumeration(int n, int k) {
    vector<vector<int>> ans;
    vector<int> tmp;
    makeEnumUtil(ans, tmp, 0, n, k);
    return ans;
}

void Utils::getLoadDist(ConvertibleCode &code, ClusterSettings &settings, vector<vector<int> > &solutions, vector<int> &send_load_dist, vector<int> &recv_load_dist) {
    int num_nodes = settings.M;

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
        int from_node_id = solution[2];
        int to_node_id = solution[3];

        send_load_dist[from_node_id] += 1;
        recv_load_dist[to_node_id] += 1;
    }

    return;
}