import os
import sys
import subprocess
import argparse
import time
import random
import numpy as np
from pathlib import Path
import numpy as np
from scipy.stats import t
import re

def parse_args(cmd_args):
    arg_parser = argparse.ArgumentParser(description="extract logs and generate data script") 
    arg_parser.add_argument("-log_filename", type=str, required=True, help="log filename")
    
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

def student_t_dist(samples_arr, ci=0.95):
    samples = np.array(samples_arr)
    # sample mean
    mean = samples.mean()
    # sample standard deviation
    stdev = samples.std()
    # degree of freedom = # of samples - 1
    dof = len(samples) - 1
    # t value
    t_crit = np.abs(t.ppf((1-ci) / 2, dof))
    print('t = {:.3f} when percentage = {:.3f} and degree of freedom = {:d}'.format(t_crit, (1-ci)*100/2, dof))
    # interval
    t_diff = stdev * t_crit / np.sqrt(len(samples))

    return mean, t_diff



def max_load_to_second(max_load, block_size_MiB=64):
    return max_load * (block_size_MiB * 1024 * 1024 * 8) / (10 ** 6)

def main():
    args = parse_args(sys.argv[1:])
    if not args:
        exit()

    # read bandwidth
    log_filename = args.log_filename 

    print("log_filename: {}".format(log_filename))

    log_path = Path(log_filename).resolve()

    max_load_token = "max_weighted_load"
    bw_token = "bandwidth"

    # baselines
    baselines = ["RDPM", "BWPM", "BTPM", "BTWeighted"]
    baseline_results = {
        "RDPM": {
            "bw": [],
            "max_load": []
        },
        "BWPM": {
            "bw": [],
            "max_load": []
        },
        "BTPM": {
            "bw": [],
            "max_load": []
        },
        "BTWeighted": {
            "bw": [],
            "max_load": []
        }
    }
    baseline_cnter = 0

    with open(log_path, "r") as f:
        for line in f.readlines():
            if max_load_token in line and bw_token in line:
                match = re.search(r"{}: (\d+.\d+), {}: (\d+.\d+)".format(max_load_token, bw_token), line)
                if not match.groups():
                    print("Error: cannot find max_load and bw_load")
                    exit()
                max_load = float(match.group(1))
                bw = float(match.group(2))

                # append results
                baseline_results[baselines[baseline_cnter]]["max_load"].append(max_load)
                baseline_results[baselines[baseline_cnter]]["bw"].append(bw)

                # increment results
                baseline_cnter = (baseline_cnter + 1) % len(baselines)

    # print results
    print(baseline_results)

    for bl_name, bl_result in baseline_results.items():
        # get results under student's t-dist
        bl_result["max_load"] = student_t_dist(bl_result["max_load"])
        bl_result["bw"] = student_t_dist(bl_result["bw"])

        # polish (mean, lower, upper)
        bl_result["max_load"] = bl_result["max_load"][0], bl_result["max_load"][0] + bl_result["max_load"][1], bl_result["max_load"][0] - bl_result["max_load"][1]
        bl_result["bw"] = bl_result["bw"][0], bl_result["bw"][0] - bl_result["bw"][1], bl_result["bw"][0] + bl_result["bw"][1]

        # polish (max_load to second)
        bl_result["max_load"] = [max_load_to_second(item) for item in bl_result["max_load"]]

    # print results
    print(baseline_results)


    
if __name__ == '__main__':
    main()
