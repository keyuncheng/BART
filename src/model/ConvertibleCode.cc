#include "ConvertibleCode.hh"

ConvertibleCode::ConvertibleCode(/* args */)
{
}

ConvertibleCode::ConvertibleCode(size_t k_in, size_t m_in, size_t k_out, size_t m_out) {
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

    printf("Convertible Code (%ld, %ld) -> (%ld, %ld), alpha: %ld, beta: %ld, theta: %ld, lambda_i: %ld, lambda_f: %ld\n", k_i, m_i, k_f, m_f, alpha, beta, theta, lambda_i, lambda_f);

}

ConvertibleCode::~ConvertibleCode()
{
}

size_t ConvertibleCode::lcm(size_t a, size_t b) {

    size_t max_num = max(a, b);

    while (1) {
        if (max_num % a == 0 && max_num % b == 0) {
            break;
        }
        max_num++;
    }

    return max_num;
}

bool ConvertibleCode::isValidForPM() {
    return (lambda_f == 1) && (m_f <= m_i);
}