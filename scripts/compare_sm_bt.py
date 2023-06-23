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
    exp_name = "exp_a1_1"
    # ks = [4,6,8,12,16]
    # ms = [2,3,4]
    # kmlpairs = [(6, 3, 3), (8, 4, 2)]
    kmlpairs = [(4,2,2), (6,2,2), (8,2,2), (6,3,2), (8,3,2), (12,3,2), (8,4,2), (12,4,2), (16,4,2)]
    # lambdas = [2,3,4]
    methods = ["RDPM", "BWPM", "BTPM"]

    # num_nodes = [100, 200, 300, 400]
    num_nodes = [100]
    num_stripes = [10000]
    num_runs = 5
    num_runs_per_placement = 5
    pre_placement_filename = "pre_placement"
    post_placement_filename = "post_placement"
    sg_meta_filename = "sg_meta"

    root_dir = Path(os.getcwd()) / ".."
    bin_dir = root_dir / "build"

    # log dir
    log_dir = root_dir / "log" / exp_name
    log_dir.mkdir(exist_ok=True, parents=True)

    # cmd = "rm -rf {}/*".format(log_dir)
    # exec_cmd(cmd)

    for kmlpair, num_node, num_stripe in itertools.product(kmlpairs, num_nodes, num_stripes):
        k = kmlpair[0]
        m = kmlpair[1]
        lambda_ = kmlpair[2]

        # num_stripe
        num_stripe = int(num_stripe/lambda_)*lambda_

        log_filename = log_dir / "{}_{}_{}_{}_{}.log".format(k, m, lambda_, num_node, num_stripe)

        exp_pre_placement_filename = bin_dir / "{}_{}_{}_{}_{}_{}".format(pre_placement_filename, k, m, lambda_, num_node, num_stripe)
        exp_post_placement_filename = bin_dir / "{}_{}_{}_{}_{}_{}".format(post_placement_filename, k, m, lambda_, num_node, num_stripe)
        exp_sg_meta_filename = bin_dir / "{}_{}_{}_{}_{}_{}".format(sg_meta_filename, k, m, lambda_, num_node, num_stripe)

        cmd = "echo \"\" > {}".format(log_filename)
        exec_cmd(cmd, exec=False)

        for i in range(num_runs):
            # generate placement
            cmd = "cd {}; ./GenPrePlacement {} {} {} {} {} {} {}".format(str(bin_dir), k, m, lambda_ * k, m, num_node, num_stripe, str(exp_pre_placement_filename))
            exec_cmd(cmd, exec=False)

            for method in methods:
                # run simulation
                cmd = "cd {}; time ./BTSGenerator {} {} {} {} {} {} {} {} {} {} >> {}".format(str(bin_dir), k, m, lambda_ * k, m, num_node, num_stripe, method, str(exp_pre_placement_filename), str(exp_post_placement_filename), str(exp_sg_meta_filename), str(log_filename))
                for each_run in range(num_runs_per_placement):
                    exec_cmd(cmd, exec=False)

if __name__ == "__main__":
    main()
