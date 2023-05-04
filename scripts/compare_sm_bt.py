import os
import sys
import itertools
import subprocess

def main():
    # ks = [2,3,4,5,6,8,16]
    ks = [16]
    ms = [2,3,4]
    # lambdas = [2,3,4]
    lambdas = [3]
    methods = ["SM", "BT"]
    num_runs = 3
    num_stripes = [300]
    num_nodes_times = [2]
    placement = "placement"
    root_dir = "/home/kycheng/Documents/projects/redundancy-transition/load-balance/BalancedConversion"
    bin_dir = "{}/build".format(root_dir)

    cmd = "cd {}".format(bin_dir)
    print(cmd)
    return_str, stderr = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).communicate()

    for k, m, lambda_, num_stripe, num_nodes_time in itertools.product(ks, ms, lambdas, num_stripes, num_nodes_times):

        if (k < m):
            continue

        for i in range(num_runs):
            # generate placement
            cmd = "cd {}; ./GenPlacement {} {} {} {} {} {} {}".format(bin_dir, k, m, lambda_ * k, m, num_stripe, num_nodes_time * (lambda_ * k + m), placement)
            print(cmd)

            return_str, stderr = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).communicate()
            msg = return_str.decode().strip()
            print(msg)

            for method in methods:
                cmd = "cd {}; ./Simulator {} {} {} {} {} {} {} {}".format(bin_dir, k, m, lambda_ * k, m, num_stripe, num_nodes_time * (lambda_ * k + m), placement, method)
                print(cmd)

                return_str, stderr = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).communicate()
                msg = return_str.decode().strip()
                print(msg)

if __name__ == "__main__":
    main()