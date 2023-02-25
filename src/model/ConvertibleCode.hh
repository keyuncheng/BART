#ifndef __CONVERTIBLE_CODE_HH__
#define __CONVERTIBLE_CODE_HH__

#include "../include/include.hh"

class ConvertibleCode
{
private:
    /* data */
public:

        // input ec parameters (k, m)
    size_t k_i;
    size_t m_i;
    size_t n_i;

    // output ec paramters (k', m')
    size_t k_f;
    size_t m_f;
    size_t n_f;

    // k' = alpha k + beta
    size_t alpha;
    size_t beta;
    
    // theta = lcm(k, k')
    size_t theta;
    size_t lambda_i;
    size_t lambda_f;

    ConvertibleCode(/* args */);
    ConvertibleCode(size_t k_in, size_t m_in, size_t k_out, size_t m_out);
    ~ConvertibleCode();

    bool isValidForPM();

private:
    size_t lcm(size_t a, size_t b);
};

#endif // __CONVERTIBLE_CODE_HH__