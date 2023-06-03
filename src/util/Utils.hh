#ifndef __UTILS_HH__
#define __UTILS_HH__

#include <iterator>
#include "../include/include.hh"

class Utils
{
private:
    /* data */
public:
    Utils(/* args */);
    ~Utils();

    static mt19937 createRandomGenerator();
    static size_t randomUInt(size_t l, size_t r, mt19937 &random_generator);

    template <typename T>
    static void printVector(T &vec)
    {
        std::copy(vec.cbegin(), vec.cend(), std::ostream_iterator<typename T::value_type>(std::cout, " "));
        printf("\n");
    }

    template <typename T>
    static vector<size_t> argsortVector(T &vec)
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

    template <typename T>
    static T lcm(T a, T b)
    {

        T max_num = max(a, b);

        while (1)
        {
            if (max_num % a == 0 && max_num % b == 0)
            {
                break;
            }
            max_num++;
        }

        return max_num;
    }

    template <typename T>
    static void dotAddVectors(T &a, T &b, T &result)
    {
        if (typeid(T) == typeid(u16string))
        {
            std::transform(a.begin(), a.end(), b.begin(), result.begin(), std::plus<uint16_t>());
        }
        else if (typeid(T) == typeid(u32string))
        {
            std::transform(a.begin(), a.end(), b.begin(), result.begin(), std::plus<uint32_t>());
        }
    }

    template <typename T>
    static void dotSubVectors(T &a, T &b, T &result)
    {
        if (typeid(T) == typeid(u16string))
        {
            std::transform(a.begin(), a.end(), b.begin(), result.begin(), std::minus<uint16_t>());
        }
        else if (typeid(T) == typeid(u32string))
        {
            std::transform(a.begin(), a.end(), b.begin(), result.begin(), std::minus<uint32_t>());
        }
    }

    static uint64_t calCombSize(uint32_t n, uint32_t k);
    static uint32_t *genAllCombs(uint32_t n, uint32_t k);
    static void getNextComb(uint32_t n, uint32_t k, u32string &cur_comb);
    static void getNextPerm(uint16_t n, uint16_t k, u16string &cur_perm);

    /**
     * @brief Get the combination given it's lexigraphical position
     *
     * e.g. for Comb(5,3) (5 choose 3), the lexigraphical position is as follows:
     * Comb | position
     * [1,1,1,0,0]: 0
     * [1,1,0,1,0]: 1
     * [1,1,0,0,1]: 2
     * ...
     * [0,0,1,1,1]: 9
     *
     * Reference: https://math.stackexchange.com/questions/1368526/fast-way-to-get-a-combination-given-its-position-in-reverse-lexicographic-or
     *
     * @param n
     * @param k
     * @param comb_id
     * @return u32string
     */
    static u32string getCombFromPosition(uint32_t n, uint32_t k, uint64_t comb_id);

    // old
    static void printUIntVector(vector<size_t> &vec);
    static void printUIntArray(size_t *arr, size_t len);
    static void printUIntMatrix(size_t **matrix, size_t r, size_t c);

    static size_t **initUIntMatrix(size_t r, size_t c);
    static void destroyUIntMatrix(size_t **matrix, size_t r, size_t c);

    static vector<size_t> argsortIntVector(vector<int> &vec);
    static vector<size_t> argsortUIntVector(vector<size_t> &vec);

    static void makeCombUtil(vector<vector<size_t>> &ans,
                             vector<size_t> &tmp, size_t n, size_t left, size_t k);
    static vector<vector<size_t>> getCombinations(size_t n, size_t k);

    static void makePermUtil(vector<vector<size_t>> &ans,
                             vector<size_t> tmp, size_t left, size_t right, size_t k);
    static vector<vector<size_t>> getPermutation(size_t n, size_t k);

    // static void getLoadDist(ConvertibleCode &code, ClusterSettings &settings, vector<vector<size_t>> &solutions, vector<size_t> &send_load_dist, vector<size_t> &recv_load_dist);

    /**
     * @brief Dot add two unsigned int vectors
     *
     * @param v1
     * @param v2
     * @return vector<size_t>
     */
    static vector<size_t> dotAddUIntVectors(vector<size_t> &v1, vector<size_t> &v2);
};

#endif // __UTILS_HH__