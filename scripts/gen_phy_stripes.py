import os
import sys
import subprocess
import argparse
import configparser
from pathlib import Path

def parse_args(cmd_args):
    arg_parser = argparse.ArgumentParser(description="generate physical stripes")
    arg_parser.add_argument("-config_filename", type=str, required=True, help="configuration file name")
    arg_parser.add_argument("-gen_meta", type=bool, default=False, help="whether to generate metadata files (placement and block mapping)")
    arg_parser.add_argument("-gen_data", type=bool, default=False, help="whether to generate data in Agents")

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

    # read args
    is_gen_meta = args.gen_meta
    is_gen_data = args.gen_data

    # Common
    k_i = int(config["Common"]["k_i"])
    m_i = int(config["Common"]["m_i"])
    k_f = int(config["Common"]["k_f"])
    m_f = int(config["Common"]["m_f"])
    num_stripes = int(config["Common"]["num_stripes"])
    num_nodes = int(config["Common"]["num_nodes"])
    enable_HDFS = False if int(config["Common"]["enable_HDFS"]) == 0 else True
    block_size = int(config["Common"]["block_size"])

    print("(k_i, m_i): ({}, {}); num_nodes: {}; num_stripes: {}; enable_HDFS: {}".format(k_i, m_i, num_stripes, num_nodes, enable_HDFS))

    # Controller
    agent_ips = config["Controller"]["agent_ips"].split(",")
    placement_filename = config["Controller"]["placement_filename"]
    block_mapping_filename = config["Controller"]["block_mapping_filename"]

    print("agent_ips: {}; placement_filename: {}; block_mapping_filename: {}".format(agent_ips, placement_filename, block_mapping_filename))

    # Others
    root_dir = Path("..").absolute()
    metadata_dir = root_dir / "metadata"
    metadata_dir.mkdir(exist_ok=True, parents=True)
    bin_dir = root_dir / "build"
    placement_path = metadata_dir / placement_filename
    block_mapping_path = metadata_dir / block_mapping_filename
    data_dir = root_dir / "data"

    # Generate placement file
    if is_gen_meta == True:
        print("generate placement placement file {}")
        cmd = "cd {}; ./GenPlacement {} {} {} {} {} {} {}".format(str(bin_dir), k_i, m_i, k_f, m_f, num_nodes, num_stripes, placement_path)
        exec_cmd(cmd, exec=True)
    
    # Read placement
    placement = []
    with open("{}".format(str(placement_path)), "r") as f:
        for line in f.readlines():
            stripe_indices = [int(block_id) for block_id in line.strip().split(" ")]
            placement.append(stripe_indices)
    
    # Generate block mapping file
    block_mapping = []

    if is_gen_meta == True:
        print("generate block mapping file {}")
        for stripe_id, stripe_indices in enumerate(placement):
            for block_id, placed_node_id in enumerate(stripe_indices):
                block_placement_path = ""
                if enable_HDFS == False:
                    block_placement_path = data_dir / "block_{}_{}".format(stripe_id, block_id)
                else:
                    pass # TO IMPLEMENT
                block_mapping.append([stripe_id, block_id, placed_node_id, block_placement_path])

    with open("{}".format(str(block_mapping_path)), "w") as f:
        for stripe_id, block_id, node_id, block_placement_path in block_mapping:
            f.write("{} {} {} {}\n".format(stripe_id, block_id, node_id, str(block_placement_path)))

    
    # Generate physical blocks (if HDFS not enabled)
    if (is_gen_data == True and enable_HDFS == False):
        print("generate physical blocks on storage nodes")

        # Read node to block mapping
        node_block_mapping = {}
        for node_id in range(num_nodes):
            node_block_mapping[node_id] = []

        with open("{}".format(str(block_mapping_path)), "r") as f:
            for line in f.readlines():
                item = line.strip().split(" ")
                stripe_id = int(item[0])
                block_id = int(item[1])
                node_id = int(item[2])
                block_placement_path = item[3]
                node_block_mapping[node_id].append(block_placement_path)

        for node_id in range(num_nodes):
            node_ip = agent_ips[node_id]
            blocks_to_gen = node_block_mapping[node_id]
            print("Generate {} blocks at Node {} ({})".format(len(blocks_to_gen), node_id, node_ip))

            cmd = "ssh {} \"mkdir -p {}\"".format(node_ip, str(data_dir))
            exec_cmd(cmd, exec=False)

            cmd = "ssh {} \"rm -f {}/*\"".format(node_ip, str(data_dir))
            exec_cmd(cmd, exec=False)

            for block_path in blocks_to_gen:
                block_size_MB = int(block_size / (1024 * 1024))
                cmd = "ssh {} \"dd if=/dev/urandom of={} bs={}M count=1 iflag=fullblock\"".format(node_ip, block_path, block_size_MB)
                exec_cmd(cmd, exec=False)
        

if __name__ == '__main__':
    main()