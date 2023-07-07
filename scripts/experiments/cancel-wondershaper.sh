#!/bin/bash

# source "./common.sh"
source "experiments/common.sh"

ssh ${USER}@${MIDDILEWARE_CONTROLLER} "cd ~/wondershaper && echo 'Infocom24@BART' | sudo -S ./wondershaper -c -a eth0"

node_index=0
for node in ${AGENTNODES[@]}; do
    echo $node " --- " $node_index
    ssh ${USER}@${node} "cd ~/wondershaper && echo 'Infocom24@BART' | sudo -S ./wondershaper -c -a eth0"
    ((node_index+=1))
done
