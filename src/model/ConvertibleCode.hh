#ifndef __CONVERTIBLE_CODE_HH__
#define __CONVERTIBLE_CODE_HH__

#include "../include/include.hh"
#include "../util/Utils.hh"

class ConvertibleCode
{
private:
    /* data */
public:
    // input ec parameters (k, m)
    uint8_t k_i;
    uint8_t m_i;
    uint8_t n_i;

    // output ec paramters (k', m')
    uint8_t k_f;
    uint8_t m_f;
    uint8_t n_f;

    // k' = alpha k + beta
    uint8_t alpha;
    uint8_t beta;

    // theta = lcm(k, k')
    uint8_t theta;
    uint8_t lambda_i;
    uint8_t lambda_f;

    ConvertibleCode(/* args */);
    ConvertibleCode(uint8_t k_in, uint8_t m_in, uint8_t k_out, uint8_t m_out);
    ~ConvertibleCode();
    void print();

    bool isValidForPM();

private:
    uint8_t lcm(uint8_t a, uint8_t b);
};

#endif // __CONVERTIBLE_CODE_HH__