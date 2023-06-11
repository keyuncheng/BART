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