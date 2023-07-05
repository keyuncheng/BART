#!/bin/bash

source "experiments/common.sh"

node_index=0
for node in ${AGENTNODES[@]}; do
    echo $node " --- " $node_index
    ssh ${USER}@${node} "cd ~/wondershaper && sudo ./wondershaper -c -a eth0"
    ssh ${USER}@${node} "cd ~/wondershaper && sudo ./wondershaper -a eth0 -u ${BANDWIDTH} -d ${BANDWIDTH}"
done
