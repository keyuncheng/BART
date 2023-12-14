import csv
import numpy as np

K = 32
M = 4
F = 4
bw_upper = 1600.0
bw_gap = 2.0
bw_lower = bw_upper / bw_gap
B_SCALE = 10

start_idx = 5

bws = []

for j in range((K+M)//F):
    idx = j*F + 1
    np.random.seed()
    temp = np.random.normal(bw_lower + B_SCALE, B_SCALE, (1, K + M - F)).tolist()[0]
    bw = []
    i = 0
    for x in range(K + M):
        if (x + 1) >= idx and (x + 1) < idx + F:
            bw.append(bw_upper)
        else:
            bw.append(temp[i])
            i += 1
    bws.append(bw)

# bw
sfilename = str("bw")+ \
            str("K")+str(K)+ \
            str("M")+str(M)+ \
            str("F")+str(F)+ \
            str("r") +str(int(bw_gap)) + \
            str(".csv")
print(sfilename)

with open(sfilename, 'w') as csvwritefile:
    writer = csv.writer(csvwritefile)
    for bw in bws:
        writer.writerow(bw)


import os
import sys
import subprocess
import argparse
import time
import random
import configparser
from pathlib import Path

def parse_args(cmd_args):
    arg_parser = argparse.ArgumentParser(description="generate bandwidth distribution")
    arg_parser.add_argument("-config_filename", type=str, required=True, help="configuration file name")
    arg_parser.add_argument("-bw_MBs_max", type=int, required=True, help="configuration file name")
    

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

    # read bandwidth
    bw_MBs_max = args.bw_MBs_max

    # read config
    config_filename = args.config_filename
    config = configparser.ConfigParser()
    config.read(config_filename)

    # Common
    num_nodes = int(config["Common"]["num_nodes"])

    # Controller
    bw_filename = config["Controller"]["bw_filename"]

    print("num_nodes: {}; bw_filename: {}".format(num_nodes, bw_filename))

    root_dir = Path("..").absolute()
    config_dir = root_dir / "config"
    bw_path = config_dir / Path(bw_filename).name

    # Read pre-transition placement
    bw_uploads = []
    bw_downloads = []

    # generate bandwidth distributions
    bw_MBs_min = bw_MBs_max / 1000

    # uniform distribution
    np.random.seed()
    bw_uploads = np.random.uniform(bw_MBs_min, bw_MBs_max, (1, num_nodes)).tolist()
    bw_downloads = np.random.uniform(bw_MBs_min, bw_MBs_max, (1, num_nodes)).tolist()

    # normal distribution (TBD)
    # np.random.seed()


    with open("{}".format(str(bw_path)), "w") as f:
        for bw_upload in bw_uploads:
            f.write("{} ".format(bw_upload))
        f.write("\n")
        for bw_download in bw_downloads:
            f.write("{} ".format(bw_download))
        f.write("\n")
    
if __name__ == '__main__':
    main()