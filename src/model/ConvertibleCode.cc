#include "ConvertibleCode.hh"

ConvertibleCode::ConvertibleCode(/* args */)
{
}

ConvertibleCode::ConvertibleCode(int k_in, int m_in, int k_out, int m_out) {
    k_i = k_in;
    m_i = m_in;

    k_f = k_out;
    m_f = m_out;

    n_i = k_in + m_in;
    n_f = k_out + m_out;
    alpha = k_f / k_i;
    beta = k_f % k_i;
    
    theta = lcm(k_i, k_f);
    lambda_i = theta / k_i;
    lambda_f = theta / k_f;

    printf("Convertible Code (%d, %d) -> (%d, %d), alpha: %d, beta: %d, theta: %d, lambda_i: %d, lambda_f: %d\n", k_i, m_i, k_f, m_f, alpha, beta, theta, lambda_i, lambda_f);

}

ConvertibleCode::~ConvertibleCode()
{
}

int ConvertibleCode::lcm(int a, int b) {

    int max_num = max(a, b);

    while (1) {
        if (max_num % a == 0 && max_num % b == 0) {
            break;
        }
        max_num++;
    }

    return max_num;
}
