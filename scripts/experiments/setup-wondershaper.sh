#!/bin/bash

# source "./common.sh"
source "experiments/common.sh"

if [[ $# -ne 1 ]]; then
  echo "Please specify the bandwidth"
  exit
fi

bandwidth=$1

ssh ${USER}@${MIDDILEWARE_CONTROLLER} "cd ~/wondershaper && echo 'Infocom24@BART' | sudo -S ./wondershaper -a eth0 -u ${bandwidth} -d ${bandwidth}"

node_index=0
for node in ${AGENTNODES[@]}; do
    echo $node " --- " $node_index
    ssh ${USER}@${node} "cd ~/wondershaper && echo 'Infocom24@BART' | sudo -S ./wondershaper -a eth0 -u ${bandwidth} -d ${bandwidth}"
    ((node_index+=1))
done
