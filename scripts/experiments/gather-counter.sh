#!/bin/bash

source "experiments/common.sh"

# start agents
node_index=0
for node in ${AGENTNODES[@]}; do
  # echo $node " --- " $node_index
  # ssh ${USER}@${node} "grep -on "readBlockCounter" ~/node_agent_${node_index}.log | wc -l"
  # ssh ${USER}@${node} "grep -on "writeBlockCounter" ~/node_agent_${node_index}.log | wc -l"
  # ssh ${USER}@${node} "grep -on "sendBlockCounter" ~/node_agent_${node_index}.log | wc -l"
  ssh ${USER}@${node} "grep -on "recvBlockCounter" ~/node_agent_${node_index}.log | wc -l"
  # ssh ${USER}@${node} "grep -on "ecEncodeDataCounter" ~/node_agent_${node_index}.log | wc -l"

  ((node_index+=1))
  # echo -e "\n"
done
