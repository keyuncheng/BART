import os
import sys
import subprocess
import argparse
import configparser
import json
import re
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

def run_cmd(args_list):
    # import subprocess
    print('Running system command: {0}'.format(' '.join(args_list)))
    proc = subprocess.Popen(
        args_list, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    s_output, s_err = proc.communicate()
    s_return = proc.returncode
    return s_return, s_output, s_err

hadoop_home = "/home/bart/hadoop-3.3.4"
hadoop_data_dir = ""

class hdfsFile:
    def __init__(self, file_id, file_size, file_ecpolicy, file_blocknum):
        self.file_id = file_id
        self.file_size = file_size
        self.file_ecpolicy = file_ecpolicy
        self.file_blocknum = file_blocknum

    def __str__(self):
        return json.dumps(self)


class Block:
    def __init__(self, block_name, block_size, block_replica):
        self.block_name = block_name
        self.block_size = block_size
        self.block_replica = block_replica
        
        block_id = self.block_name.split('_')[1]
        self.block_id = (int)(block_id)
        d1 = ((self.block_id >> 16) & 0x1F)
        d2 = ((self.block_id >> 8) & 0x1F)
        self.block_location = os.path.join(
            "/home/bart/hadoop-3.3.4/tmp/dfs/data/current", hadoop_data_dir, "current/finalized", "subdir" + str(d1), "subdir" + str(d2))

    def __str__(self):
        return json.dumps(self)



class BlockLoc:
    def __init__(self, location_name, location_dn, location_ds, hadoop_data_dir):
        self.location_name = location_name
        self.location_dn = location_dn
        self.location_ds = location_ds
        self.location_id = (int)(self.location_name.split('_')[1])
        d1 = ((self.location_id >> 16) & 0x1F)
        d2 = ((self.location_id >> 8) & 0x1F)
        self.location_path = os.path.join(
            "/home/bart/hadoop-3.3.4/tmp/dfs/data/current", hadoop_data_dir, "current/finalized", "subdir" + str(d1), "subdir" + str(d2))

datanode_map = {}

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

    # Controller
    agent_addrs = config["Controller"]["agent_addrs"].split(",")
    pre_placement_filename = config["Controller"]["pre_placement_filename"]
    pre_block_mapping_filename = config["Controller"]["pre_block_mapping_filename"]

    # Agent
    block_size = int(config["Agent"]["block_size"])

    print("(k_i, m_i): ({}, {}); num_nodes: {}; num_stripes: {}".format(k_i, m_i, num_stripes, num_nodes))

    print("pre_placement_filename: {}; pre_block_mapping_filename: {}".format(pre_placement_filename, pre_block_mapping_filename))

    # Others
    root_dir = Path("..").absolute()
    metadata_dir = root_dir / "metadata"
    metadata_dir.mkdir(exist_ok=True, parents=True)
    bin_dir = root_dir / "build"
    pre_placement_path = metadata_dir / pre_placement_filename
    pre_block_mapping_path = metadata_dir / pre_block_mapping_filename
    data_dir = root_dir / "data"

    for i, addr in enumerate(agent_addrs):
        datanode_map[addr.split(":")[0]] = str(i)
    print(datanode_map)

    # HDFS
    enable_HDFS = False if int(config["Common"]["enable_HDFS"]) == 0 else True
    hadoop_transitioning_path = config["HDFS"]["hadoop_transitioning_path"]
    hadoop_home = config["HDFS"]["hadoop_home"]
    if enable_HDFS:
        file2blocks = {}
        block2locations = {}

        # execute scripts
        (ret, out, err) = run_cmd(
            ['hdfs', 'fsck', os.path.join(hadoop_transitioning_path), '-locations', '-files', '-blocks'])
        lines = out.decode("utf-8").split('\n')

        filekey = ""
        blockkey = ""

        for line in lines:
            # file description line
            if re.search("^" + hadoop_transitioning_path + "/", line):
                file_meta_list = re.split('=|,| |: ', line)
                if file_meta_list[11] == "OK":
                    filekey = file_meta_list[0]
                    file2blocks[filekey] = []
                    filesize = file_meta_list[1]
                    fileecpolicy = file_meta_list[6]
                    fileblocknum = file_meta_list[8]
                else:
                    print("Block error! Stop processing.")
                    exit(-1)
            elif re.search("^[0-9]+\.", line):
                block_file_list = re.split('\[|\]|,', line.strip(' '))
                block_meta = re.split(' ', block_file_list[0].strip())
                block_data_dir = block_meta[1].split(":")[0]
                block = Block(block_meta[1], block_meta[2].split(
                    "=")[1], block_meta[3].split("=")[1])
                if filekey != "":
                    file2blocks[filekey].append(block)
                    blockkey = block.block_name
                    block2locations[blockkey] = []
                for blockindex in range(int(block_meta[3].split("=")[1])):
                    location = BlockLoc(
                        re.split(':', block_file_list[blockindex*5+1])[0].strip(),
                        block_file_list[blockindex*5+2].strip(),
                        block_file_list[blockindex*5+3].strip(),
                        block_data_dir
                    )
                    if blockkey != "":
                        block2locations[blockkey].append(location)
        pre_block_mapping_hdfs_path = Path(str(pre_block_mapping_path) + "_hdfs")
        with open(pre_placement_path, 'w') as ppf, open(pre_block_mapping_path, 'w') as pmf, open(pre_block_mapping_hdfs_path, 'w') as pmhf:
            fileindex=0
            for filename in file2blocks:
                for blockgroup in file2blocks[filename]:
                    locindex=0
                    for loc in block2locations[blockgroup.block_name]:
                        locdn = datanode_map[loc.location_dn.split(":")[0]]
                        locpath = loc.location_path + "/blk_" + str(loc.location_id)
                        locpath_hdfs = loc.location_path + "/blk_" + str(loc.location_id) + "_" + blockgroup.block_name.split("_")[2]
                        ppf.write(locdn + " ")
                        pmf.write(str(fileindex) + " " + str(locindex) + " " + locdn + " " + locpath)
                        pmhf.write(str(fileindex) + " " + str(locindex) + " " + locdn + " " + locpath_hdfs)
                        locindex = locindex + 1
                        pmf.write("\n")
                        pmhf.write("\n")
                fileindex = fileindex + 1
                ppf.write("\n")
        exit(0)

    # Generate pre-transition stripe placement file
    print("generate pre-transition placement file {}".format(str(pre_placement_path)))

    cmd = "cd {}; ./GenPrePlacement {} {} {} {} {} {} {}".format(str(bin_dir), k_i, m_i, k_f, m_f, num_nodes, num_stripes, str(pre_placement_path))
    exec_cmd(cmd, exec=True)
    
    # Read pre-transition stripe placement file
    pre_placement = []
    with open("{}".format(str(pre_placement_path)), "r") as f:
        for line in f.readlines():
            stripe_indices = [int(block_id) for block_id in line.strip().split(" ")]
            pre_placement.append(stripe_indices)
    
    # obtain pre_block_mapping from pre_placement
    pre_block_mapping = []
    for stripe_id, stripe_indices in enumerate(pre_placement):
        for block_id, placed_node_id in enumerate(stripe_indices):
            # # Original Version: place the actual data on node_{}
            # pre_block_placement_path = data_dir / "node_{}".format(placed_node_id) / "block_{}_{}".format(stripe_id, block_id)

            # Hacked Version: only store one stripe in data_dir
            pre_block_placement_path = data_dir / "block_{}_{}".format(0, block_id)
            
            pre_block_mapping.append([stripe_id, block_id, placed_node_id, pre_block_placement_path])

    # Write block mapping file
    print("generate pre-transition block mapping file {}".format(str(pre_block_mapping_path)))

    with open("{}".format(str(pre_block_mapping_path)), "w") as f:
        for stripe_id, block_id, node_id, pre_block_placement_path in pre_block_mapping:
            f.write("{} {} {} {}\n".format(stripe_id, block_id, node_id, str(pre_block_placement_path)))
        

if __name__ == '__main__':
    main()