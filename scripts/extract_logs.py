import os
import sys
import subprocess
import argparse
import itertools
import re
from pathlib import Path

def parse_args(cmd_args):
    arg_parser = argparse.ArgumentParser(description="extract logs")
    args = arg_parser.parse_args(cmd_args)
    return args

def exec_cmd(cmd, exec=False):
    print("Execute Command: {}".format(cmd))
    msg = ""
    if exec == True:
        return_str, stderr = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).communicate()
        msg = return_str.decode().strip()
        print(msg)
    return msg

def main():
    args = parse_args(sys.argv[1:])
    if not args:
        exit()

    ks = [4,6,8,12,16]
    ms = [2,3,4]
    lambdas = [2,3,4]
    methods = ["RDRE", "BWRE", "BTRE", "RDPM", "BWPM", "BTPM", "BT"]
    # ks = [4,6]
    # ms = [4]
    # lambdas = [3]
    # methods = ["RDRE", "RDPM", "BWRE", "BWPM"]
    # methods = ["BTRE", "BTPM", "BT"]
    num_runs = 5
    num_nodes_times = [2]
    num_stripes = [1000]
    root_dir = Path(os.getcwd()) / ".."
    log_dir = root_dir / "log"

    max_load_dict = {}
    bandwidth_dict = {}

    for k, m, lambda_, num_nodes_time, num_stripe in itertools.product(ks, ms, lambdas, num_nodes_times, num_stripes):
        if (k < m):
            continue

        # num_nodes
        num_nodes = num_nodes_time * (lambda_ * k + m)

        # num_stripe
        if num_stripe % lambda_ > 0:
            num_stripe = (int(num_stripe / lambda_) + 1) * lambda_

        # extract the logs
        log_filename = "{}_{}_{}_{}_{}.log".format(k, m, lambda_, num_nodes, num_stripe)
        # log_filename = "{}_{}_{}_{}_{}_BT.log".format(k, m, lambda_, num_nodes, num_stripe)
        log_filepath = log_dir / log_filename

        if (log_filepath.exists() == False):
            continue

        # init the results
        result_key = "{}_{}_{}_{}_{}".format(k, m, lambda_, num_nodes, num_stripe)
        max_load_dict[result_key] = [0.0 for method in methods]
        bandwidth_dict[result_key] = [0.0 for method in methods]

        with open(str(log_filepath), "r") as rf:
            result_lines = []

            for line in rf.readlines():
                if "max_load" in line:
                    result_lines.append(line)

            if len(result_lines) < num_runs * len(methods):
                print("error: not enough results_lines for key: {}".format(result_key))
                continue

            # sum up the results
            for run_id in range(num_runs):
                for method_id, method in enumerate(methods):
                    result_line = result_lines[run_id * len(methods) + method_id]
                    
                    match_obj = re.match(r"max_load: (.*), bandwidth: (.*)", result_line)

                    if not match_obj:
                        print("error: not matched!")
                        continue
                    max_load = float(match_obj.groups()[0])
                    bandwidth = float(match_obj.groups()[1])

                    max_load_dict[result_key][method_id] += max_load
                    bandwidth_dict[result_key][method_id] += bandwidth

            # average
            max_load_dict[result_key] = [1.0 * max_load / num_runs for max_load in max_load_dict[result_key]]
            bandwidth_dict[result_key] = [1.0 * bandwidth / num_runs for bandwidth in bandwidth_dict[result_key]]
    
    print(max_load_dict)
    print(bandwidth_dict)


    


if __name__ == '__main__':
    main()