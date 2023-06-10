import os
import sys
import subprocess
import argparse
import configparser
from pathlib import Path

def parse_args(cmd_args):
    arg_parser = argparse.ArgumentParser(description="generate metadata for post-transition stripes with pre-transition stripes")
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
    num_nodes = int(config["Common"]["num_nodes"])
    num_stripes = int(config["Common"]["num_stripes"])
    approach = config["Common"]["approach"]
    enable_HDFS = False if int(config["Common"]["enable_HDFS"]) == 0 else True

    # Controller
    pre_placement_filename = config["Controller"]["pre_placement_filename"]
    pre_block_mapping_filename = config["Controller"]["pre_block_mapping_filename"]
    post_placement_filename = config["Controller"]["post_placement_filename"]
    post_block_mapping_filename = config["Controller"]["post_block_mapping_filename"]
    sg_meta_filename = config["Controller"]["sg_meta_filename"]

    print("(k_i, m_i): ({}, {}); num_nodes: {}; num_stripes: {}; approach: {}, enable_HDFS: {}".format(k_i, m_i, num_stripes, num_nodes, approach, enable_HDFS))

    print("pre_placement_filename: {}; pre_block_mapping_filename: {}; post_placement_filename: {}; post_block_mapping_filename: {}, sg_meta_filename: {}".format(pre_placement_filename, pre_block_mapping_filename, post_placement_filename, post_block_mapping_filename, sg_meta_filename))

    if enable_HDFS == True:
        print("pre-transition placement and block mapping should be obtained from HDFS; skipped")
        return

    # Others
    lambda_ = int(k_f / k_i)
    num_post_stripes = int(num_stripes / lambda_)

    root_dir = Path("..").absolute()
    metadata_dir = root_dir / "metadata"
    metadata_dir.mkdir(exist_ok=True, parents=True)
    bin_dir = root_dir / "build"
    pre_placement_path = metadata_dir / pre_placement_filename
    pre_block_mapping_path = metadata_dir / pre_block_mapping_filename
    post_placement_path = metadata_dir / post_placement_filename
    post_block_mapping_path = metadata_dir / post_block_mapping_filename
    sg_meta_path = metadata_dir / sg_meta_filename
    data_dir = root_dir / "data"
    
    # Read pre-transition placement
    pre_placement = []
    with open("{}".format(str(pre_placement_path)), "r") as f:
        for line in f.readlines():
            stripe_indices = [int(block_id) for block_id in line.strip().split(" ")]
            pre_placement.append(stripe_indices)
    
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
            pre_block_mapping[pre_stripe_id][pre_block_id] = pre_block_placement_path


    # Perform transition
    print("generate transition solution, post-transition placement file: {}".format(str(post_placement_path)))
    cmd = "cd {}; ./BTSGenerator {} {} {} {} {} {} {} {} {} {}".format(str(bin_dir), k_i, m_i, k_f, m_f, num_nodes, num_stripes, approach,  str(pre_placement_path), str(post_placement_path), str(sg_meta_path))
    exec_cmd(cmd, exec=True)

    # Read post placement
    post_placement = []
    with open("{}".format(str(post_placement_path)), "r") as f:
        for line in f.readlines():
            stripe_indices = [int(block_id) for block_id in line.strip().split(" ")]
            post_placement.append(stripe_indices)

    # Read stripe group metadata
    sg_meta = []
    with open("{}".format(str(sg_meta_path)), "r") as f:
        for line in f.readlines():
            line_int = [int(item) for item in line.split()]
            sg_record = {}
            sg_record["pre_stripe_ids"] = line_int[0:lambda_]
            sg_record["enc_method"] = line_int[lambda_]
            sg_record["enc_nodes"] = line_int[lambda_+1:]
            sg_meta.append(sg_record)

    # Generate post-transition block mapping
    post_block_mapping = []
    for post_stripe_id, post_stripe_indices in enumerate(post_placement):
        for post_block_id, placed_node_id in enumerate(post_stripe_indices):
            post_block_placement_path = None
            if post_block_id < k_f:
                # data block: read from pre_block_mapping
                pre_stripe_id = int(post_block_id / k_i)
                pre_block_id = int(post_block_id % k_i)
                pre_stripe_id_global = sg_meta[post_stripe_id]["pre_stripe_ids"][pre_stripe_id]
                post_block_placement_path = data_dir / "node_{}".format(placed_node_id) / Path(pre_block_mapping[pre_stripe_id_global][pre_block_id]).name
            else:
                if enable_HDFS:
                    pass # TO IMPLEMENT
                else:
                    # parity block: generate new filename
                    post_block_placement_path = data_dir / "node_{}".format(placed_node_id) / "post_block_{}_{}".format(post_stripe_id, post_block_id)
            post_block_mapping.append([post_stripe_id, post_block_id, placed_node_id, post_block_placement_path])

    # Write post-transition block mapping file
    print("generate post-transition block mapping file {}".format(str(post_block_mapping_path)))

    with open("{}".format(str(post_block_mapping_path)), "w") as f:
        for stripe_id, block_id, node_id, post_block_placement_path in post_block_mapping:
            f.write("{} {} {} {}\n".format(stripe_id, block_id, node_id, str(post_block_placement_path)))
        

if __name__ == '__main__':
    main()