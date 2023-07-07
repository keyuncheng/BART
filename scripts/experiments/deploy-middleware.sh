#!/bin/bash

source "experiments/common.sh"

# modify config and sync
num_nodes=30

num_nodes_provided=${#AGENTNODES[@]}
if [[ $num_nodes -ne $num_nodes_provided ]]; then
  echo $num_nodes_provided
  echo $num_nodes
  echo "error in node list provided "
  exit
fi


bash experiments/sync.sh ${MIDDLEWARE_HOME_PATH}/conf/config.ini
bash experiments/sync.sh ${MIDDLEWARE_HOME_PATH}/build/Agent

# start controller
ssh ${USER}@${MIDDILEWARE_CONTROLLER} "nohup sh -c 'cd ${MIDDLEWARE_HOME_PATH}/build && ./Controller ../conf/config.ini' > controller.log 2>&1 &"

# start agents
node_index=0
for node in ${AGENTNODES[@]}; do
  echo $node
  echo "nohup sh -c 'cd ${MIDDLEWARE_HOME_PATH}/build && ./Agent ${node_index} ../conf/config.ini' > node_agent_${node_index}.log 2>&1 &"
  ssh ${USER}@${node} "nohup sh -c 'cd ${MIDDLEWARE_HOME_PATH}/build && ./Agent ${node_index} ../conf/config.ini' > node_agent_${node_index}.log 2>&1 &"
  ((node_index+=1))
done
