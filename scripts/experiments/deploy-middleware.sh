#!/bin/bash

source "./common.sh"

# modify config and sync
num_nodes=20

num_nodes_provided=${#AGENTNODES[@]}
if [[ $num_nodes -ne $num_nodes_provided ]]; then
  echo $num_nodes_provided
  echo $num_nodes
  echo "error in node list provided "
  exit
fi

num_stripes=1000
approach=BT
block_size=67108864
controller_addr=10.10.10.11:10001
agent_addrs=10.10.10.12:10001,10.10.10.13:10001,10.10.10.15:10001,10.10.10.16:10001,10.10.10.17:10001,10.10.10.18:10001,10.10.10.19:10001,10.10.10.20:10001,10.10.10.22:10001,10.10.10.23:10001,10.10.10.24:10001,10.10.10.25:10001,10.10.10.26:10001,10.10.10.27:10001,10.10.10.28:10001,10.10.10.29:10001,10.10.10.30:10001,10.10.10.31:10001,10.10.10.32:10001,10.10.10.33:10001

sed -i "s/num_nodes = .*/num_nodes = $num_nodes/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
sed -i "s/num_stripes = .*/num_stripes = $num_stripes/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
sed -i "s/approach = .*/approach = $approach/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
sed -i "s/block_size = .*/block_size = $block_size/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
sed -i "s/controller_addr = .*/controller_addr = $controller_addr/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
sed -i "s/agent_addrs = .*/agent_addrs = $agent_addrs/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini

bash sync.sh ${MIDDLEWARE_HOME_PATH}/conf/config.ini

# start controller
ssh ${USER}@${MIDDILEWARE_CONTROLLER} "nohup sh -c 'cd ${MIDDLEWARE_HOME_PATH}/build && ./Controller ../conf/config.ini' > controlller.log 2>&1 &"


# start agents
node_index=0
for node in ${AGENTNODES[@]}; do
  # ssh ${USER}@${node} "nohup sh -c 'cd ${MIDDLEWARE_HOME_PATH}/build && rm -r * && cmake .. && make' > output.log 2>&1 &"
  echo $node
  echo "nohup sh -c 'cd ${MIDDLEWARE_HOME_PATH}/build && ./Agent ${node_index} ../conf/config.ini' > node_agent.log 2>&1 &"
  ssh ${USER}@${node} "nohup sh -c 'cd ${MIDDLEWARE_HOME_PATH}/build && ./Agent ${node_index} ../conf/config.ini' > node_agent.log 2>&1 &"
  ((node_index+=1))
done
