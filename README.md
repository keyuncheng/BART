# BalancedConversion
Balanced Code Conversion

## Workflow

* Balanced Transition Generator (implemented, verified)
    * Read stripe placement
    * Generate transition solution

* Controller
    * Generate physical stripes (implemented, verified)
        * Use middleware: store in ```/data/``` with file name
          ```block_<stripe_id>_<block_id>```
        * Use HDFS: call HDFS put to write stripes
            * Metadata is obtained from HDFS
    * Maintain metadata (implemented, verified)
        * Block placement: ```/metadata/placement```
            * Each line represents a (k, m) stripe of length: k + m
            * Line format: <node_id_0> <node_id_1> ... <node_id_k+m-1>
        * Block mapping: ```/metadata/block_mapping```
            * It's a verbose version of Placement
            * format: each line stores the metadata of a block:
            * <stripe_id> <block_id> <placed_node_id> <absolute_path>
    * Read stripe metadata (implemented, verified)
        * Placement
        * Block mapping
    * Generate transition solution with BTS (implemented, verified)
    * Parse the transition solution from BTS into commands (implemented,
      verified)
    * Distribute the transition commands to Agents (implemented, verified)
    * Wait for all finish signals from all Agents (implemented, verified)
        * Report the time (implemented, verified)

* Agent
    * Handle the commands
        * From Controller: Parse Transfer command into Send command, and send
          physical blocks to other Agents (implemented, verified)
        * From Agents: Parse Send command and receive physical block
          (implemented, verified)
    * Pipeline
        * Read -> Compute (implemented, verified)
        * Transfer -> Compute (implemented, verified)
        * Transfer -> Write (implemented, verified)
        * Compute -> Write (implemented, verified)
        * Delete: directly delete (implemented, verified)


## Install

The software is built on Sockpp (socket programming) for node communications
and ISA-L for erasure coding.

### Required Libraries

#### General

You can install with apt-get to install the following packages

```
sudo apt-get install g++ make cmake yasm nasm autoconf libtool git
```

Note that Sockpp requires the cmake version of v3.12 or above

#### Sockpp

Github: [link](https://github.com/fpagliughi/sockpp.git)

You may try to deploy with the instructions on the Github repository, or with
the following instructions:

```
git clone https://github.com/fpagliughi/sockpp.git
cd sockpp
mkdir build ; cd build
cmake ..
make
sudo make install
sudo ldconfig
```

#### ISA-L

Github: link(https://github.com/intel/isa-l)

You may try to deploy with the instructions on the Github repository, or with
the following instructions:

```
./autogen.sh
./configure
make
sudo make install
```

### Build

```
mkdir build; cd build
cmake ..
make
```

### Modules

We deploy the system over ```n + 1``` nodes (1 NameNode (Controller node) and
n DataNodes (Agent node)). Please make sure the [configurations
files](#configuration-file) are the same across the cluster.

#### Configuration file

modify ```conf/config.ini``` for the configurations

```
EC Parametrs: (k_i, m_i), (k_f, m_f)
Cluster settings: num_nodes, num_stripes, addresses (ip:port)
Transition Metadata absolute paths (stored on the Controller): <*>_filename
```

#### BTSGenerator

A module to generate transition solution.

First, generate pre-stripe placements and metadata

```
cd scripts; python3 gen_pre_stripes.py -config_filename ../conf/config.ini
```

Second, use BTSGenerator to generate the transition solution, and generate
corresponding post-stripe placements and metadata

```
python3 gen_post_stripes_meta.py -config_filename ../conf/config.ini
```

Third, distribute sample EC stripes to nodes
```
python3 distribute_blocks.py -config_filename ../conf/config.ini
```

### Controller

On the Controller node, run the executable

```
./Controller <config_filename>
```

### Agent

On each Agent node, run the executable

```
./Agent <agent_id> <config_filename>
```

After Controller and Agents all starts, the transitioning starts, and ends
after all Agents finished operations. The transitioning time will be reported
on each node.