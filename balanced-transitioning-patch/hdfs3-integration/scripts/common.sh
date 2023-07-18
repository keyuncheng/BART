#!/bin/bash

USER="hcpuyang"
HOME_DIR="/home/${USER}"

DATANODES=("cloud-node2" "cloud-node3" "cloud-node5" "cloud-node6" "cloud-node7" "cloud-node8" "cloud-node9" "cloud-node10" "cloud-node12" "cloud-node13" "cloud-node14" "cloud-node15" "cloud-node16" "cloud-node17" "cloud-node18" "cloud-node19" "cloud-node20" "cloud-node21" "cloud-node22" "cloud-node23")
AGENTNODES=("cloud-node2" "cloud-node3" "cloud-node5" "cloud-node6" "cloud-node7" "cloud-node8" "cloud-node9" "cloud-node10" "cloud-node12" "cloud-node13" "cloud-node14" "cloud-node15" "cloud-node16" "cloud-node17" "cloud-node18" "cloud-node19" "cloud-node20" "cloud-node21" "cloud-node22" "cloud-node23")

MAPPING_FILE_PATH="./placementandmapping/"

## middleware configs
MIDDILEWARE_CONTROLLER="cloud-node1"
MIDDLEWARE_HOME_PATH="${HOME_DIR}/BalancedConversion/"
