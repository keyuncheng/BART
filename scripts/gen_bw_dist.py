import os
import sys
import subprocess
import argparse
import time
import random
import configparser
import numpy as np
from pathlib import Path

def parse_args(cmd_args):
    arg_parser = argparse.ArgumentParser(description="generate bandwidth distribution")
    arg_parser.add_argument("-config_filename", type=str, required=True, help="configuration file name")
    arg_parser.add_argument("-bw_min", type=int, required=True, help="bandwidth min value (MB/s)")
    arg_parser.add_argument("-bw_max", type=int, required=True, help="bandwidth max value (MB/s)")    
    

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
    bw_min = args.bw_min
    bw_max = args.bw_max

    # read config
    config_filename = args.config_filename
    config = configparser.ConfigParser()
    config.read(config_filename)

    # Common
    num_nodes = int(config["Common"]["num_nodes"])

    # Controller
    bw_filename = config["Controller"]["bw_filename"]

    print("num_nodes: {}; bw_filename: {}".format(num_nodes, bw_filename))

    root_dir = Path("..").resolve()
    config_dir = root_dir / "conf"
    bw_path = config_dir / Path(bw_filename).name

    # Read pre-transition placement
    bw_uploads = []
    bw_downloads = []

    # uniform distribution
    np.random.seed()
    bw_uploads = np.random.uniform(bw_max, bw_min, (1, num_nodes)).tolist()
    bw_downloads = np.random.uniform(bw_max, bw_min, (1, num_nodes)).tolist()

    with open("{}".format(str(bw_path)), "w") as f:
        for bw_upload in bw_uploads[0]:
            f.write("{} ".format(bw_upload))
        f.write("\n")
        for bw_download in bw_downloads[0]:
            f.write("{} ".format(bw_download))
        f.write("\n")
    
if __name__ == '__main__':
    main()