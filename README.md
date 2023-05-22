# BalancedConversion
Balanced Code Conversion

## Workflow

* Controller
    * Generate physical stripes (done)
        * Use middleware: store in ```/data/``` with file name
          ```blk_<stripe_id>_<block_id>```
        * Use HDFS: call HDFS put to write stripes
    * Store stripe metadata (done)
        * Placement: ```/metadata/placement```
            * format: each line represents a (k, m) stripe of length: k + m;
              store the node ids
            * <node_id_0> <node_id_1> ... <node_id_k+m-1>
        * Block mapping: ```/metadata/block_mapping```
            * It's a verbose version of Placement
            * format: each line stores the metadata of a block:
            * <stripe_id> <block_id> <node_id> <absolute_path>
    * Read stripe metadata
        * Placement
        * Block mapping
    * Generate transition solution with BTS (done)
    * Parse the transition solution from BTS into commands
    * Distribute the transition commands to Agents (done)
    * Wait for all finish signals from all Agents (done)
        * Report the time

* Balanced Transition Generator (done)
    * Read stripe placement
    * Generate transition solution

* Agent
    * Handle the commands
        * From Controller: Parse Transfer command into Send command, and send
          physical blocks to other Agents
        * From Agents: Parse Send command and receive physical block
    * Command Format:
        * Read: stripe_id, block_id, final_block_id, physical path
            * Cache into memory
        * Write: stripe_id, block_id, final_block_id, physical path
            * From memory to path
        * Transfer: stripe_id, block_id, final_block_id, src_node_id,
          dst_node_id
          * Send: same format
        * Compute: stripe_id, final_block_id
        * Delete: stripe_id, block_id, physical path
    * Pipeline
        * Read -> Compute
        * Transfer -> Compute
        * Transfer -> Write
        * Compute -> Write
        * Delete: directly delete