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
    exp_name = "exp_a4"
    # ks = [4,6,8,12,16]
    # ms = [2,3,4]
    # lambdas = [2,3,4]
    kmlpairs = [(6,3,3)]
    # kmlpairs = [(4,2,2), (6,2,2), (8,2,2), (6,3,2), (8,3,2), (12,3,2), (8,4,2), (12,4,2), (16,4,2)]
    methods = ["RDPM", "BWPM", "BTPM", "BTWeighted"]
    # methods = ["BTPM"]

    num_nodes = [100]
    num_stripes = [12000]
    
    bw_lowers = [10, 50, 100, 500]
    bw_upper = 1000

    num_runs = 30
    num_runs_per_placement = 1
    pre_placement_filename = "pre_placement"
    post_placement_filename = "post_placement"
    sg_meta_filename = "sg_meta"
    bw_filename = "bw_profile"

    root_dir = Path(os.getcwd()) / ".."
    bin_dir = root_dir / "build"
    script_dir = root_dir / "scripts"
    conf_dir = root_dir / "conf"

    # bw file
    bw_filepath = conf_dir / bw_filename

    # log dir
    log_dir = root_dir / "log" / exp_name
    log_dir.mkdir(exist_ok=True, parents=True)

    # cmd = "rm -rf {}/*".format(log_dir)
    # exec_cmd(cmd)

    for kmlpair, num_node, num_stripe, bw_lower in itertools.product(kmlpairs, num_nodes, num_stripes, bw_lowers):
        k = kmlpair[0]
        m = kmlpair[1]
        lambda_ = kmlpair[2]

        # num_stripe
        num_stripe = int(num_stripe/lambda_)*lambda_

        log_filename = log_dir / "{}_{}_{}_{}_{}_{}.log".format(k, m, lambda_, num_node, num_stripe, bw_lower)

        exp_pre_placement_filename = bin_dir / "{}_{}_{}_{}_{}_{}_{}".format(pre_placement_filename, k, m, lambda_, num_node, num_stripe, bw_lower)
        exp_post_placement_filename = bin_dir / "{}_{}_{}_{}_{}_{}_{}".format(post_placement_filename, k, m, lambda_, num_node, num_stripe, bw_lower)
        exp_sg_meta_filename = bin_dir / "{}_{}_{}_{}_{}_{}_{}".format(sg_meta_filename, k, m, lambda_, num_node, num_stripe, bw_lower)
        
        exp_bw_filename = bin_dir / "{}_{}_{}_{}_{}_{}_{}".format(bw_filename, k, m, lambda_, num_node, num_stripe, bw_lower)

        cmd = "echo \"\" > {}".format(log_filename)
        exec_cmd(cmd, exec=True)

        for i in range(num_runs):
            # generate placement
            cmd = "cd {}; ./GenPrePlacement {} {} {} {} {} {} {}".format(str(bin_dir), k, m, lambda_ * k, m, num_node, num_stripe, str(exp_pre_placement_filename))
            exec_cmd(cmd, exec=True)

            # generate bw profile
            cmd = "cd {}; python3 gen_bw_dist.py -num_nodes {} -bw_min {} -bw_max {} -bw_filename {}".format(script_dir, num_node, bw_lower, bw_upper, exp_bw_filename)
            exec_cmd(cmd, exec=True)

            for method in methods:
                # run simulation
                cmd = "cd {}; time ./BTSGenerator {} {} {} {} {} {} {} {} {} {} {} >> {}".format(str(bin_dir), k, m, lambda_ * k, m, num_node, num_stripe, method, str(exp_pre_placement_filename), str(exp_post_placement_filename), str(exp_sg_meta_filename), str(exp_bw_filename), str(log_filename))
                for each_run in range(num_runs_per_placement):
                    exec_cmd(cmd, exec=True)

if __name__ == "__main__":
    main()
