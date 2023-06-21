import os
import sys
import subprocess
import argparse

def parse_args(cmd_args):
    arg_parser = argparse.ArgumentParser(description="generate pre-transition stripes")
    arg_parser.add_argument("-eck", type=int, required=True, help="eck")
    arg_parser.add_argument("-ecm", type=int, required=True, help="ecm")
    arg_parser.add_argument("-eclambda", type=int, required=True, help="eclambda")    
    arg_parser.add_argument("-M", type=int, required=True, help="number of storage nodes")

    args = arg_parser.parse_args(cmd_args)
    return args

def factorial(n):
    res = 1
    for i in range(1, n + 1):
        res *= i
    return res

def comb(n, k):
    return perm(n,k) / factorial(k)

def perm(n, k):
    return factorial(n) / factorial(n - k)

def num_perfect_pm(eck, ecm, eclambda, M):
    result = 1
    for i in range(1, eclambda):
        result *= comb((M - i * eck - ecm), eck)
    return result


def main():
    args = parse_args(sys.argv[1:])
    if not args:
        exit()

    # read params
    eck = args.eck
    ecm = args.ecm
    eclambda = args.eclambda
    M = args.M

    total_num_placements = comb(M, eck + ecm) * perm(eck + ecm, ecm)
    total_num_stripe_groups = pow(total_num_placements, eclambda)
    total_num_perfect_pm_stripe_groups = total_num_placements * num_perfect_pm(eck, ecm, eclambda, M)
    prob_perfect_pm = 1.0 * total_num_perfect_pm_stripe_groups / total_num_stripe_groups
    
    print("results_1:")
    print("total_num_placements: {}", total_num_placements)
    print("total_num_stripe_groups: {}", total_num_stripe_groups)
    print("total_num_perfect_pm_stripe_groups: {}", total_num_perfect_pm_stripe_groups)
    print("prob_perfect_pm: {}", prob_perfect_pm)

    total_num_placements = perm(M, eck + ecm)
    total_num_stripe_groups = pow(total_num_placements, eclambda)
    total_num_perfect_pm_stripe_groups = perm(M, eclambda * eck + ecm)
    prob_perfect_pm = 1.0 * total_num_perfect_pm_stripe_groups / total_num_stripe_groups
    print("results_2:")
    print("total_num_placements: {}", total_num_placements)
    print("total_num_stripe_groups: {}", total_num_stripe_groups)
    print("total_num_perfect_pm_stripe_groups: {}", total_num_perfect_pm_stripe_groups)
    print("prob_perfect_pm: {}", prob_perfect_pm)


if __name__ == "__main__":
    main()