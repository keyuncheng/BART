# BART
BAlanced Redundancy Transitioning

## Table of Contents

* [Overview](#overview)
* [System Architecture](#system-architecture)
* [System Requirements](#system-requirements)
* [Deployment](#deployment)
* [Installation](#installation)
* [Run Simulation](#run-simulation)
* [Run Prototype](#run-prototype)

## Overview

BART is a load-balanced redundancy transitioning scheme for large-scale
erasure-coded storage, with the objective of minimizing the overall redundancy
transitioning time through carefully scheduled parallelization.

* BART generates the redundancy transitioning solutions based on the stripe
  placements
* BART performs the actual transitioning operations to change the redundancy
  (i.e., coding parameters) of existing data

## System Architecture

### Backend Storage

* HDFS (HDFS3 integration mode)
* Local FS (standalone mode)

### Middleware

* BTSGenerator (collocated with HDFS NameNode)
    * Read input stripe metadata
        * HDFS (default)
        * Local
    * Generate transitioning solution
        * output stripe metadata; stripe group metadata
    * It also serves as the **simulator**

* Controller (collocated with HDFS NameNode)
    * Parse the transitioning solution from BTSGenerator
        * input stripe metadata, output stripe metadata; stripe group metadata
    * Generate transitioning commands
        * Parity block generation (compute new parity blocks by parity
          merging)
        * Stripe re-distribution (transfer data blocks or new parity blocks)
    * Distribute the transitioning commands to Agents
    * Wait for all Agents to finish the transitioning, and transitioning time

* Agent (collocated with HDFS DataNode)
    * Handle the transitioning commands
        * Parity block generation
        * Stripe re-distribution
    * Pipeline
        * Parity block generation
            * Retrieve original parity blocks (local read; transfer from
              other Agents)
            * Compute new parity blocks
            * Write
        * Stripe re-distribution
            * Read (read locally)
            * Transfer to other Agents
            * Write

## System Requirements

* OS: Ubuntu 20.04 (tested)
* Backend storage
    * HDFS 3.3.4 ([link](https://hadoop.apache.org/docs/r3.3.4/))
* Middleware
    * Third-party libraries
        * Communication: Sockpp (socket-based library)
        ([link](https://github.com/fpagliughi/sockpp/))
        * Erasure coding: ISA-L ([link](https://github.com/intel/isa-l))

## Deployment

* We assume the following default parameters:
    * Number of storage nodes = 30
    * Number of stripes (randomly distributed) = 1200
    * Transitioning parameters: (k,m,λ) = (6,3,3) (or (k,m) = (6,3) to (18,3))
    * Block size: 64MiB
    * Network bandwidth: 1Gbps
    * Mode: HDFS3 integration mode

* Machines
    * Prepare a cluster of 30 + 1 machines in Alibaba Cloud
      ([link](https://cn.aliyun.com/))
        * Machine type: `ecs.g7.xlarge`
            * CPU: 4 vCPUs
            * Memory: 16 GiB
        * Disk type: 100GiB ESSD with level PL1
    * Default username: `bart`
    * Configure mutual password-less ssh login for all machines: [link](https://www.ibm.com/support/pages/configuring-ssh-login-without-password)
    * Configure network bandwidth: [link](#configure-network-bandwidth)
        
We provide the sample machine configs below:
    
| Machine | Number | IP |
| --- | --- | --- |
| Controller (NameNode) | 1 | 172.23.114.132 |
| Agent (DataNode) | 30 | 172.23.114.160, 172.23.114.148, 172.23.114.149, 172.23.114.157, 172.23.114.151, 172.23.114.152, 172.23.114.158, 172.23.114.159, 172.23.114.136, 172.23.114.141, 172.23.114.139, 172.23.114.162, 172.23.114.143, 172.23.114.153, 172.23.114.155, 172.23.114.150, 172.23.114.145, 172.23.114.156, 172.23.114.138, 172.23.114.140, 172.23.114.135, 172.23.114.146, 172.23.114.144, 172.23.114.163, 172.23.114.142, 172.23.114.154, 172.23.114.137, 172.23.114.134, 172.23.114.147, 172.23.114.161 |

### Configure Network Bandwidth

* We configure the network bandwidth via Wondershaper
  [link](https://github.com/magnific0/wondershaper)

* On each machine, configure the network bandwidth to 1 Gbps

```
cd wondershaper
sudo ./wondershaper -a eth0 -u 1048576 -d 1048576
```

## Installation

Please install HDFS (patched) and middleware for **ALL** machines.

* [HDFS Installation](#hdfs-installation)
* [Middleware Installation](#middleware-installation)

### HDFS Installation

Please install the below dependencies, and build HDFS with BART-patch

#### General Dependencies

Install the general dependencies with `apt-get`:
```
sudo apt-get install g++ make cmake yasm nasm autoconf libtool git
```

Note: cmake version needs to be v3.12 or above (for installing Sockpp)


#### Java

Install Java 8

```
sudo apt-get install openjdk-8-jdk
```

#### Maven

Install Maven

```
sudo apt-get install maven
```

#### ISA-L

Install ISA-L with the following instructions (you may also follow the default
instructions on [Github repo](https://github.com/intel/isa-l)):

```
./autogen.sh
./configure
make
sudo make install
```


#### Build HDFS-3.3.4 with BART-patch

* Add the following paths to `~/.bashrc`

```
# java
export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
export PATH=$JAVA_HOME/bin:$PATH

# maven
export MAVEN_HOME=/usr/share/maven
export PATH=$MAVEN_HOME/bin:$PATH

# hadoop
export HADOOP_HOME=/home/bart/hadoop-3.3.4
export HADOOP_CLASSPATH=${JAVA_HOME}/lib/tools.jar
export PATH=$HADOOP_HOME/bin:$HADOOP_HOME/sbin:$PATH
```

* Applying the changes

```
source ~/.bashrc
```

* Download Hadoop source code `hadoop-3.3.4-src.tar.gz`
  ([link](https://downloads.apache.org/hadoop/common/hadoop-3.3.4/hadoop-3.3.4-src.tar.gz))
  and extract to `/home/bart/hadoop-3.3.4-src`

* Build HDFS-3.3.4 with BART-patch

```
cd bart-hdfs3-integration
bash install.sh
```


#### HDFS configurations

* Update `$HADOOP_HOME/etc/hadoop/hdfs-site.xml`
    * Update HDFS block size `dfs.blocksize` to 64MiB
    * Add `dfs.namenode.external.metadata.path` to hdfs-site.xml

```
<configuration>
    <property><name>dfs.blocksize</name><value>67108864</value></property>
    <property><name>dfs.namenode.external.metadata.path</name><value>/home/bart/jsonfile/</value></property>
</configuration>
```

* Copy the Vandermonde based RS(6,3) configuration file to
   `$HADOOP_HOME/etc/hadoop/`
    * Reference: RS-LEGACY in HDFS [link](https://hadoop.apache.org/docs/stable/hadoop-project-dist/hadoop-hdfs/HDFSErasureCoding.html)

```
cp bart-hdfs3-integration/etc/user_ec_policies_rs_legacy_6_3.xml $HADOOP_HOME/etc/hadoop
```

* For the other unspecified parameters, we use the default settings.


### Middleware Installation

Please install the below dependencies, and build the BART prototype.

#### Sockpp

Install with the following instructions (you may also follow the default
instructions on [Github repo](https://github.com/fpagliughi/sockpp/))

```
git clone https://github.com/fpagliughi/sockpp.git
cd sockpp
mkdir build ; cd build
cmake ..
make
sudo make install
sudo ldconfig
```


#### Build

Build the middleware with the following instructions

```
mkdir build; cd build
cmake ..
make
```

#### Configuration

We provide the sample configuration file `conf/config.ini` for the default
settings. Please **copy** the configuration files to all nodes.

We list the configuration parameters in `conf/config.ini` as below:

| Parameters | Description | Example |
| --- | --- | --- |
| Common |
| k_i | Input k | `6` |
| m_i | Input m | `3` |
| k_f | Output k (or λ * input k) | `18` |
| m_f | Output m (the same as input m) | `3` |
| num_nodes | Number of storage nodes | `30` |
| num_stripes | Number of stripes | `1200` |
| approach | Transitioning solution | `BTPM` (for BART); `BWPM` (for Bandwidth-driven solution); `RDPM` (for randomized solution) |
| enable_HDFS | whether to perform transitioning from HDFS (or local storage) | `true` (HDFS); `false` (local storage) |
| Controller |
| controller_addr | Address of Controller (IP:port) | `172.23.114.132:10001` |
| agent_addrs | Address of all Agents (IP:port) | `172.23.114.160:10001,172.23.114.148:10001,172.23.114.149:10001,172.23.114.157:10001,172.23.114.151:10001,172.23.114.152:10001,172.23.114.158:10001,172.23.114.159:10001,172.23.114.136:10001,172.23.114.141:10001,172.23.114.139:10001,172.23.114.162:10001,172.23.114.143:10001,172.23.114.153:10001,172.23.114.155:10001,172.23.114.150:10001,172.23.114.145:10001,172.23.114.156:10001,172.23.114.138:10001,172.23.114.140:10001,172.23.114.135:10001,172.23.114.146:10001,172.23.114.144:10001,172.23.114.163:10001,172.23.114.142:10001,172.23.114.154:10001,172.23.114.137:10001,172.23.114.134:10001,172.23.114.147:10001,172.23.114.161:10001` |
| pre_placement_filename | Input stripe metadata (stripe placement) | `/home/bart/BART/metadata/pre_placement` |
| pre_block_mapping_filename | Input stripe metadata (block to physical path mapping) | `/home/bart/BART/metadata/pre_block_mapping` |
| post_placement_filename | Output stripe metadata (stripe placement) | `/home/bart/BART/metadata/post_placement` |
| post_block_mapping_filename | Output stripe metadata (block to physical path mapping) | `/home/bart/BART/metadata/post_block_mapping` |
| sg_meta_filename | Stripe group metadata (grouping information, encoding method and encoding nodes) | `/home/bart/BART/metadata/post_block_mapping` |
| Agent |
| block_size | Block size (If HDFS is enabled, should be consistent with HDFS configurations) | `67108864` |
| num_compute_workers | Number of compute worker threads | `10` |
| num_reloc_workers | Number of relocation worker threads | `10` |
| HDFS | should setup when HDFS is enabled |
| hadoop_namenode_addr | NameNode IP address | `172.23.114.132` |
| hadoop_home | HDFS home directory | `/home/bart/hadoop-3.3.4` |
| hadoop_cell_size | HDFS cell size (should be consistent with HDFS ec policy) | `1048576` (default HDFS cell size) |
| hadoop_transitioning_path | the HDFS directory storing the input stripes | `/ec_test` |
| hdfs_file_prefix | File prefixes of HDFS files | `testfile` |
| hdfs_file_size | File size (input k * block size) | `402653184` |
| metadata_file_path | HDFS file metadata (after transitioning) | `/home/bart/jsonfile/` |
| block_id_start | HDFS block metadata | `-922337203685477500` (by default) |
| block_group_id_start | HDFS block metadata | `2000` (by default) |


## Run Simulation

* Generate input stripe metadata (into `pre_placement`)

```
cd build
./GenPrePlacement 6 3 18 3 30 1200 pre_placement
```

* Generate the transitioning solution (output stripe metadata into
  `post_placement` and stripe group metadata `sg_meta`)

```
./BTSGenerator 6 3 18 3 30 1200 BTPM pre_placement post_placement sg_meta
```

We can find the load distribution, max load, transitioning bandwidth and other
stats from the console output.

```
max_load: 61, bandwidth: 1577
```

## Run Prototype

### Generate Input Stripes

* We write 1200 input stripes to HDFS with RS(6,3)

* Prepare the directory (`/ec_test`) for writing EC stripes

```
hdfs ec -addPolicies -policyFile ${HADOOP_HOME}/etc/hadoop/user_ec_policies_rs_legacy_6_3.xml
hdfs ec -enablePolicy -policy RS-LEGACY-6-3-1024k
hadoop fs -mkdir /ec_test
hdfs ec -setPolicy -path /ec_test -policy RS-LEGACY-6-3-1024k
```

We can check whether the EC policy has been successfully applied to `/ec_test`:

```
hdfs ec -getPolicy -path /ec_test
```

* We write a single stripe as follows
    * Create random files of `hdfs_file_size` (k * block size = 6 * 64MiB = 402653184)
    * Write the file to HDFS (HDFS randomly distributes the blocks by default)

```
dd if=/dev/urandom of=testfile1 bs=64MiB count=6
hdfs dfs -put testfile1 /ec_test
```


### Generate Transitioning Solution

* Generate input stripe metadata

```
cd scripts
python3 gen_pre_stripes.py -config_filename ../conf/config.ini
```

* Generate the transitioning solution (output stripe metadata and stripe group
  metadata)

```
python3 gen_post_stripes_meta.py -config_filename ../conf/config.ini
```

* Parse output stripe metadata for HDFS to support data retrieval after
  transitioning. The metadata is stored in `/home/bart/jsonfile/`

```
python3 recover_post_placement.py -config_filename ../conf/config.ini
```

### Run Controller

On the Controller node, run the executable

```
./Controller ../conf/config.ini
```

### Run Agent

On each Agent node `<agent_id> (from 0 to 29)`, run the executable

```
./Agent <agent_id> ../conf/config.ini
```

### Transitioning Time

The transitioning begins after we start the Controller and Agents, and
finishes after the Controller and all Agents have finished the execution. The
transitioning time will be reported on each node.

```
Controller::main finished transitioning, time: xxx ms
Agent::main finished transitioning, time: xxx ms
```

### Data Retrieval after Transitioning

We can retrieve the data after transitioning (named `testfile1_out`), and
compare with the original data `testfile1`

```
hdfs dfs -get /ec_test/testfile1 testfile1_out
diff testfile1 testfile1_out
```


### Misc: Run in Standalone Mode

if we use standalone mode (enable_HDFS = false), we run the following scripts
to generate input stripes:

* Generate input stripe metadata

```
cd scripts
python3 gen_pre_stripes.py -config_filename ../conf/config.ini
```

* Generate the transitioning solution (output stripe metadata and stripe group
  metadata)

```
python3 gen_post_stripes_meta.py -config_filename ../conf/config.ini
```

* Generate and distribute the physical stripes

```
python3 distribute_blocks.py -config_filename ../conf/config.ini
```

We start the redundancy transitioning by [run Controller](#run-controller) and
[run Agent](#run-agent)