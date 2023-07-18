import subprocess
import re
import os
import sys
import json

query_filename = ""
qeury_path = ""

file2blocks = {}
block2locations = {}

# Should be obtained from config file
data_dir_prefix = "/home/hcpuyang/hadoop-3.3.4/tmp/dfs/data/current"
data_dir_subfix = "current/finalized"
data_dir = ""

if (len(sys.argv) < 2):
    print("Please provide the querying path, filename:")
    print(" python3 gen_pre_placement.py <path> [filename]")
    exit(-1)
elif (len(sys.argv) == 3):
    query_path = sys.argv[1]
    query_filename = sys.argv[2]
else:
    query_path = sys.argv[1]

datanode_map = {
    "12": "00",
    "13": "01",
    "15": "02",
    "16": "03",
    "17": "04",
    "18": "05",
    "19": "06",
    "20": "07",
    "22": "08",
    "23": "09",
    "24": "10",
    "25": "11",
    "26": "12",
    "27": "13",
    "28": "14",
    "29": "15",
    "30": "16",
    "31": "17",
    "32": "18",
    "33": "19",
}


def run_cmd(args_list):
    """
      run linux commands
    """
    # import subprocess
    print('Running system command: {0}'.format(' '.join(args_list)))
    proc = subprocess.Popen(
        args_list, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    s_output, s_err = proc.communicate()
    s_return = proc.returncode
    return s_return, s_output, s_err


class hdfsFile:
    def __init__(self, file_id, file_size, file_ecpolicy, file_blocknum):
        self.file_id = file_id
        self.file_size = file_size
        self.file_ecpolicy = file_ecpolicy
        self.file_blocknum = file_blocknum

    def print(self):
        print("File info:")
        print("\t", self.file_id)
        print("\t", self.file_size)
        print("\t", self.file_ecpolicy)
        print("\t", self.file_blocknum)

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
            data_dir_prefix, data_dir, data_dir_subfix, "subdir" + str(d1), "subdir" + str(d2))

    def getid(self):
        return self.block_id

    def print(self):
        print("block info:")
        print("\t", self.block_name)
        print("\t", self.block_size)
        print("\t", self.block_replica)
        print("\t", self.block_id)
        print("\t", self.block_location)

    def __str__(self):
        return json.dumps(self)



class BlockLoc:
    def __init__(self, location_name, location_dn, location_ds, data_dir):
        self.location_name = location_name
        self.location_dn = location_dn
        self.location_ds = location_ds
        self.location_id = (int)(self.location_name.split('_')[1])
        d1 = ((self.location_id >> 16) & 0x1F)
        d2 = ((self.location_id >> 8) & 0x1F)
        self.location_path = os.path.join(
            data_dir_prefix, data_dir, data_dir_subfix, "subdir" + str(d1), "subdir" + str(d2))


    def print(self):
        print("location info:")
        print("\t", self.location_name)
        print("\t", self.location_dn, " ", os.path.join(
            data_dir_prefix, data_dir, data_dir_subfix, self.location_path))



# Create a dictionary with values of MyCustomClass objects


def main():
    # execute scripts
    (ret, out, err) = run_cmd(
        ['hdfs', 'fsck', os.path.join(query_path, query_filename), '-locations', '-files', '-blocks'])
    lines = out.decode("utf-8").split('\n')

    filekey = ""
    blockkey = ""

    for line in lines:
        # file description line
        if re.search("^" + query_path + "/", line):
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
            data_dir = block_meta[1].split(":")[0]
            block = Block(block_meta[1], block_meta[2].split(
                "=")[1], block_meta[3].split("=")[1])
            # block.print()
            if filekey != "":
                file2blocks[filekey].append(block)
                blockkey = block.block_name
                block2locations[blockkey] = []
            for blockindex in range(int(block_meta[3].split("=")[1])):
                location = BlockLoc(
                    re.split(':', block_file_list[blockindex*5+1])[0].strip(),
                    block_file_list[blockindex*5+2].strip(),
                    block_file_list[blockindex*5+3].strip(),
                    data_dir
                )
                # location.print()
                if blockkey != "":
                    block2locations[blockkey].append(location)


    pre_placement_filename = ""
    pre_mapping_filename = ""
    if query_filename == "":
        pre_placement_filename = "placementandmapping/pre_placement_all"
        pre_mapping_filename = "placementandmapping/pre_mapping_all"
        pre_mapping_hdfs_filename = "placementandmapping/pre_mapping_hdfs"
    else:
        pre_placement_filename = "placementandmapping/pre_placement_" + query_filename
        pre_mapping_filename = "placementandmapping/pre_mapping_" + query_filename

    with open(pre_placement_filename, 'w') as ppf, open(pre_mapping_filename, 'w') as pmf, open(pre_mapping_hdfs_filename, 'w') as pmhf:
        fileindex=0
        for filename in file2blocks:
            for blockgroup in file2blocks[filename]:
                locindex=0
                for loc in block2locations[blockgroup.block_name]:
                    locdn = datanode_map[loc.location_dn.split(".")[3].split(":")[0]]
                    locpath = loc.location_path + "/blk_" + str(loc.location_id) + "_" + blockgroup.block_name.split("_")[2]
                    locpath_hdfs = loc.location_path + "/blk_" + str(loc.location_id)
                    ppf.write(locdn + " ")
                    pmf.write(str(fileindex) + " " + str(locindex) + " " + locdn + " " + locpath)
                    pmhf.write(str(fileindex) + " " + str(locindex) + " " + locdn + " " + locpath_hdfs)
                    locindex = locindex + 1
                    pmf.write("\n")
                    pmhf.write("\n")
            fileindex = fileindex + 1
            ppf.write("\n")

if __name__ == '__main__':
    main()