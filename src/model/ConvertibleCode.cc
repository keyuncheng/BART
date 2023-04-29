#include "ConvertibleCode.hh"

ConvertibleCode::ConvertibleCode(/* args */)
{
}

ConvertibleCode::ConvertibleCode(uint8_t k_in, uint8_t m_in, uint8_t k_out, uint8_t m_out)
{
    k_i = k_in;
    m_i = m_in;

    k_f = k_out;
    m_f = m_out;

    n_i = k_in + m_in;
    n_f = k_out + m_out;
    alpha = k_f / k_i;
    beta = k_f % k_i;

    theta = Utils::lcm(k_i, k_f);
    lambda_i = theta / k_i;
    lambda_f = theta / k_f;
}

ConvertibleCode::~ConvertibleCode()
{
}

void ConvertibleCode::print()
{
    printf("Convertible Code (%u, %u) -> (%u, %u), alpha: %u, beta: %u, theta: %u, lambda_i: %u, lambda_f: %u\n", k_i, m_i, k_f, m_f, alpha, beta, theta, lambda_i, lambda_f);
}

bool ConvertibleCode::isValidForPM()
{
    return (lambda_f == 1) && (m_f <= m_i);
}