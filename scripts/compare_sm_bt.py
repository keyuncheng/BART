import os
import sys
import itertools
import subprocess
from pathlib import Path

def exec_cmd(cmd, exec=False):
    print("Execute Command: {}".format(cmd))
    msg = ""
    if exec == True:
        return_str, stderr = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).communicate()
        msg = return_str.decode().strip()
        print(msg)
    return msg

def main():
    ks = [4,6,8,12,16]
    ms = [2,3,4]
    lambdas = [2,3,4]
    methods = ["RDRE", "BWRE", "BTRE", "RDPM", "BWPM", "BTPM", "BT"]
    # ks = [4]
    # ms = [2]
    # lambdas = [3]
    # methods = ["BTRE", "BTPM", "BT"]
    num_stripes = [10000]
    num_nodes_times = [2]
    num_runs = 5
    pre_placement_filename = "pre_placement"
    post_placement_filename = "post_placement"
    sg_meta_filename = "sg_meta"

    root_dir = Path(os.getcwd()) / ".."
    bin_dir = root_dir / "build"

    # log dir
    log_dir = root_dir / "log"
    log_dir.mkdir(exist_ok=True, parents=True)

    # cmd = "rm -rf {}/*".format(log_dir)
    # exec_cmd(cmd)

    for k, m, lambda_, num_stripe, num_nodes_time in itertools.product(ks, ms, lambdas, num_stripes, num_nodes_times):

        if (k < m):
            continue

        # num_nodes
        num_nodes = num_nodes_time * (lambda_ * k + m)

        # num_stripe
        if num_stripe % lambda_ > 0:
            num_stripe = (int(num_stripe / lambda_) + 1) * lambda_

        exp_pre_placement_filename = bin_dir / "{}_{}_{}_{}_{}_{}".format(pre_placement_filename, k, m, lambda_, num_nodes, num_stripe)
        exp_post_placement_filename = bin_dir / "{}_{}_{}_{}_{}_{}".format(post_placement_filename, k, m, lambda_, num_nodes, num_stripe)
        exp_sg_meta_filename = bin_dir / "{}_{}_{}_{}_{}_{}".format(sg_meta_filename, k, m, lambda_, num_nodes, num_stripe)

        for i in range(num_runs):
            # generate placement
            cmd = "cd {}; ./GenPrePlacement {} {} {} {} {} {} {}".format(str(bin_dir), k, m, lambda_ * k, m, num_nodes, num_stripe, str(exp_pre_placement_filename))
            
            exec_cmd(cmd, exec=False)

            log_filename = log_dir / "{}_{}_{}_{}_{}.log".format(k, m, lambda_, num_nodes, num_stripe)

            for method in methods:
                # run simulation
                cmd = "cd {}; ./Simulator {} {} {} {} {} {} {} {} {} {} >> {}".format(str(bin_dir), k, m, lambda_ * k, m, num_nodes, num_stripe, method, str(exp_pre_placement_filename), str(exp_post_placement_filename), str(exp_sg_meta_filename), str(log_filename))
                
                exec_cmd(cmd, exec=False)

if __name__ == "__main__":
    main()
