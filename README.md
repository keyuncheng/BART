# BalancedConversion
Balanced Code Conversion

## Workflow

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
    * Distribute the transition commands to Agents (on-going)
    * Wait for all finish signals from all Agents (implemented)
        * Report the time

* Balanced Transition Generator (implemented, verified)
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