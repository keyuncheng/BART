import os
import json
import sys
import subprocess
import argparse
import configparser
from pathlib import Path

datanode_map = {
    "0": "12",
    "1": "13",
    "2": "15",
    "3": "16",
    "4": "17",
    "5": "18",
    "6": "19",
    "7": "20",
    "8": "22",
    "9": "23",
    "10": "24",
    "11": "25",
    "12": "26",
    "13": "27",
    "14": "28",
    "15": "29",
    "16": "30",
    "17": "31",
    "18": "32",
    "19": "33",
}

def parse_args(cmd_args):
    arg_parser = argparse.ArgumentParser(description="generate metadata files from post mapping file")
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

    k_i = int(config["Common"]["k_i"])
    m_i = int(config["Common"]["m_i"])
    k_f = int(config["Common"]["k_f"])
    m_f = int(config["Common"]["m_f"])

    file_prefix = os.path.join(config["HDFS"]["hdfs_path"], config["HDFS"]["hdfs_file_prefix"])
    file_size = int(config["HDFS"]["hdfs_file_size"])
    hadoop_namenode_addr = config["HDFS"]["hadoop_namenode_addr"]
    hadoop_home = config["HDFS"]["hadoop_home"]
    placement_file_path = config["HDFS"]["placement_file_path"]
    metadata_file_path = config["HDFS"]["metadata_file_path"]


    pool_id = "BP-" + "879262521-" + hadoop_namenode_addr + "-1685075764854"
    file_save_prefix = file_prefix.replace('/', '-')
    stripe_index = 0
    stripe_file_size = int(k_f/k_i)
    stripe_block_size = k_f + k_i

    stripe_list = []
    with open(os.path.join(placement_file_path, 'sg_meta_all'), 'r') as sg_file:
        for line in sg_file:
            parts = line.split(" ")
            stripe_list.append(int(parts[0])+1)
            stripe_list.append(int(parts[1])+1)
    
    # print(stripe_list)

    # Open the input files
    stripe_count = 0

    with open(os.path.join(placement_file_path, 'post_mapping_all'), 'r') as mapping_file:
        line_count = len(mapping_file.readlines())
        # print(line_count)
        stripe_count = int(stripe_count / (k_f + m_f))

    current_timestamp = ""

    with open(os.path.join(placement_file_path, 'post_mapping_all'), 'r') as mapping_file:
        # Create a dictionary to store the parsed data
        data = {
            'file_name': "",
            'file_size': file_size,
            'block_group_ec_policy': "RS-" + str(k_f) + "-" + str(m_f) + "-1024k",
            'block_group_list': []
        }

        line_index = 1
        file_index = 0
        for line in mapping_file:
            # Split the line into its components
            parts = line.split()

            if line_index % (k_f + m_f) == 0:
                # Read the last block in this stripe and save
                block_id = parts[3].split("_")[1]
                block_dn = "10.10.10." + datanode_map[parts[2]]
                block_path = parts[3].split("_")[0] + "_" + block_id
                block_gen_stamp = parts[3].split("_")[2]

                current_block = {
                    'block_dn': block_dn + ":9866",
                    'block_id': block_id,
                    'block_path': block_path,
                    'block_in_use': True,
                    'block_gen_stamp': block_gen_stamp
                }
                current_block_group["block_list"].append(current_block)
                data['block_group_list'].append(current_block_group)

                for local_file_index in range(0, stripe_file_size):
                    # file_name = "jsonfiles/file" + file_save_prefix + str(file_index + stripe_file_size * stripe_index) + ".json"
                    file_index = stripe_list[stripe_index*stripe_file_size + local_file_index]
                    file_name = os.path.join(metadata_file_path, "file"+file_save_prefix+str(file_index)+".json")
                    # file_name = os.path.join(metadata_file_path, "file" + file_save_prefix + str(file_index + stripe_file_size * stripe_index) + ".json")
                    data['file_name'] = file_prefix + str(file_index)
                    data['block_group_list'][0]['block_group_name'] = ""
                    print(file_name)
                    for local_block_index in range(0, k_f):
                        if int(local_block_index / k_i) == local_file_index:
                            # print("is file index {}\n".format(local_block_index))
                            data['block_group_list'][0]['block_list'][local_block_index]['block_in_use'] = True
                            if data['block_group_list'][0]['block_group_name'] == "":
                                data['block_group_list'][0]['block_group_name'] = pool_id + ":blk_" + data['block_group_list'][0]['block_list'][local_block_index]['block_id'] + "_" + current_timestamp
                        else:
                            # print("not file index {}\n".format(local_block_index))
                            data['block_group_list'][0]['block_list'][local_block_index]['block_in_use'] = False
                    print(data['block_group_list'][0]['block_group_name'])
                    print("========")
                    with open(file_name, 'w') as f:
                        json.dump(data, f)

                # Create new data for the next stripe
                data = {
                    'file_name': "",
                    'file_size': file_size,
                    'block_group_ec_policy': "RS-" + str(k_f) + "-" + str(m_f) + "-1024k",
                    'block_group_list': []
                }
                stripe_index = stripe_index + 1

            elif line_index % (k_f + m_f) == 1:
                # Set the group block info and read the first block
                block_id = parts[3].split("_")[1]
                block_dn = "10.10.10." + datanode_map[parts[2]]
                block_path = parts[3].split("_")[0] + "_" + block_id
                block_gen_stamp = parts[3].split("_")[2]
                # print(block_gen_stamp)

                current_timestamp = block_gen_stamp

                current_block_group = {
                    'block_group_name': pool_id + ":blk_" + block_id + "_" + current_timestamp,
                    'block_group_size': file_size,
                    'block_list': []
                }

                current_block = {
                    'block_dn': block_dn + ":9866", 
                    'block_id': block_id,
                    'block_path': block_path,
                    'block_in_use': True,
                    'block_gen_stamp': block_gen_stamp
                }
                current_block_group['block_list'].append(current_block)
            
            else:
                # Read the current block
                block_id = parts[3].split("_")[1]
                block_dn = "10.10.10." + datanode_map[parts[2]]
                block_path = parts[3].split("_")[0] + "_" + block_id
                block_gen_stamp = parts[3].split("_")[2]
                current_block = {
                    'block_dn': block_dn + ":9866",
                    'block_id': block_id,
                    'block_path': block_path,
                    'block_in_use': True,
                    'block_gen_stamp': block_gen_stamp
                }
                current_block_group['block_list'].append(current_block)
            line_index = line_index + 1

if __name__ == '__main__':
    main()