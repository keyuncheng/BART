import os
import sys
import subprocess
import argparse
import configparser
from pathlib import Path

def parse_args(cmd_args):
    arg_parser = argparse.ArgumentParser(description="generate pre-transition stripes")
    arg_parser.add_argument("-config_filename", type=str, required=True, help="configuration file name")

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

    # read config
    config_filename = args.config_filename
    config = configparser.ConfigParser()
    config.read(config_filename)

    # Common
    k_i = int(config["Common"]["k_i"])
    m_i = int(config["Common"]["m_i"])
    k_f = int(config["Common"]["k_f"])
    m_f = int(config["Common"]["m_f"])
    num_stripes = int(config["Common"]["num_stripes"])
    num_nodes = int(config["Common"]["num_nodes"])
    enable_HDFS = False if int(config["Common"]["enable_HDFS"]) == 0 else True
    block_size = int(config["Common"]["block_size"])

    # Controller
    agent_addrs_raw = config["Controller"]["agent_addrs"].split(",")
    agent_addrs = [item.split(":") for item in agent_addrs_raw]

    pre_placement_filename = config["Controller"]["pre_placement_filename"]
    pre_block_mapping_filename = config["Controller"]["pre_block_mapping_filename"]

    print("(k_i, m_i): ({}, {}); num_nodes: {}; num_stripes: {}; enable_HDFS: {}".format(k_i, m_i, num_stripes, num_nodes, enable_HDFS))

    print("agent_addrs: {}; pre_placement_filename: {}; pre_block_mapping_filename: {}".format(agent_addrs, pre_placement_filename, pre_block_mapping_filename))

    if enable_HDFS == True:
        print("pre-transition placement and block mapping should be obtained from HDFS; skipped")
        return

    # Others
    root_dir = Path("..").absolute()
    metadata_dir = root_dir / "metadata"
    metadata_dir.mkdir(exist_ok=True, parents=True)
    bin_dir = root_dir / "build"
    pre_placement_path = metadata_dir / pre_placement_filename
    pre_block_mapping_path = metadata_dir / pre_block_mapping_filename
    data_dir = root_dir / "data"
    
    lambda_ = int(k_f / k_i)

    
    # Read pre-stripe block mapping
    pre_block_mapping = [None] * num_stripes
    for pre_stripe_id in range(num_stripes):
        pre_block_mapping[pre_stripe_id] = [None] * (k_i + m_i)

    with open("{}".format(str(pre_block_mapping_path)), "r") as f:
        for line in f.readlines():
            line = line.split()
            pre_stripe_id = int(line[0])
            pre_block_id = int(line[1])
            pre_node_id = int(line[2])
            pre_block_placement_path = line[3]
            pre_block_mapping[pre_stripe_id][pre_block_id] = [pre_node_id, pre_block_placement_path]

    # Generate physical blocks
    print("generate physical blocks on storage nodes")

    # obtain node to block mapping
    node_block_mapping = {}
    for node_id in range(num_nodes):
        node_block_mapping[node_id] = []

    for pre_stripe_id, pre_stripes in enumerate(pre_block_mapping):
        for pre_block_id, item in enumerate(pre_stripes):
            pre_node_id, pre_block_placement_path = item
            node_block_mapping[pre_node_id].append([pre_stripe_id, pre_block_id, pre_block_placement_path])

    # generate an EC stripe group
    cmd = "cd {}; ./GenECStripe {} {} {} {} {} {}".format(str(bin_dir), k_i, m_i, k_f, m_f, block_size, str(data_dir) + "/")
    exec_cmd(cmd, exec=True)

    # # Original Version (distribute all blocks)
    # for node_id in range(num_nodes):
    #     node_ip = agent_addrs[node_id][0]
    #     blocks_to_gen = node_block_mapping[node_id]
    #     print("Generate {} blocks at Node {} ({})".format(len(blocks_to_gen), node_id, node_ip))

    #     node_dir = data_dir / "node_{}".format(node_id)

    #     cmd = "ssh {} \"mkdir -p {}\"".format(node_ip, str(node_dir))
    #     exec_cmd(cmd, exec=True)

    #     cmd = "ssh {} \"bash -c \\\"rm -rf {}/*\\\"\"".format(node_ip, str(node_dir))
    #     exec_cmd(cmd, exec=True)

    #     for pre_stripe_id, pre_block_id, pre_block_placement_path in blocks_to_gen:
    #         ec_pre_block_path = data_dir / "block_{}_{}".format(0, pre_block_id)

    #         cmd = "scp {} {}:{}".format(str(ec_pre_block_path), node_ip, pre_block_placement_path)
    #         exec_cmd(cmd, exec=True)
        

    # Hacked Version: only keep one pre-stripe across the cluster
    for node_id in range(num_nodes):
        node_ip = agent_addrs[node_id][0]

        print("Generate blocks at Node {} ({})".format(node_id, node_ip))

        node_dir = data_dir / "node_{}".format(node_id)

        cmd = "ssh {} \"mkdir -p {}\"".format(node_ip, str(node_dir))
        exec_cmd(cmd, exec=True)

        cmd = "ssh {} \"bash -c \\\"rm -rf {}/*\\\"\"".format(node_ip, str(node_dir))
        exec_cmd(cmd, exec=True)

        for pre_stripe_id in range(lambda_):
            for pre_block_id in range(k_i + m_i):
                ec_pre_block_path  = data_dir / "block_{}_{}".format(pre_stripe_id, pre_block_id)

                cmd = "scp {} {}:{}".format(str(ec_pre_block_path), node_ip, str(data_dir))
                exec_cmd(cmd, exec=True)

if __name__ == '__main__':
    main()