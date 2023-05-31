import os
import sys
import itertools
import subprocess

def main():
    ks = [4,6,8,12,16]
    ms = [2,3,4]
    lambdas = [2,3,4]
    # ks = [4]
    # ms = [2]
    # lambdas = [3]
    methods = ["RDRE", "RDPM", "BWRE", "BWPM", "BTRE", "BTPM", "BT"]
    # methods = ["BWRE", "BWPM"]
    # methods = ["BTRE", "BTPM", "BT"]
    num_runs = 5
    num_stripes = [1000]
    num_nodes_times = [2]
    placement = "placement"
    root_dir = "/home/kycheng/Documents/projects/redundancy-transition/load-balance/BalancedConversion"
    bin_dir = "{}/build".format(root_dir)

    # log dir
    log_dir = "{}/log".format(root_dir)
    if not os.path.exists(log_dir):
        os.makedirs(log_dir)
    cmd = "rm -rf {}/*".format(log_dir)
    # return_str, stderr = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).communicate()
    # msg = return_str.decode().strip()

    for k, m, lambda_, num_stripe, num_nodes_time in itertools.product(ks, ms, lambdas, num_stripes, num_nodes_times):

        if (k < m):
            continue

        num_nodes = num_nodes_time * (lambda_ * k + m)

        # num_stripe
        if num_stripe % lambda_ > 0:
            num_stripe = (int(num_stripe / lambda_) + 1) * lambda_

        for i in range(num_runs):
            # generate placement
            cmd = "cd {}; ./GenPlacement {} {} {} {} {} {} {}".format(bin_dir, k, m, lambda_ * k, m, num_nodes, num_stripe, placement)
            print(cmd)

            return_str, stderr = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).communicate()
            msg = return_str.decode().strip()
            print(msg)

            log_filename = "{}/{}_{}_{}_{}_{}.log".format(log_dir, k, m, lambda_, num_nodes, num_stripe)

            for method in methods:
                cmd = "cd {}; ./Simulator {} {} {} {} {} {} {} {} >> {}".format(bin_dir, k, m, lambda_ * k, m, num_nodes, num_stripe, placement, method, log_filename)
                print(cmd)

                return_str, stderr = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).communicate()
                msg = return_str.decode().strip()
                print(msg)

if __name__ == "__main__":
    main()
