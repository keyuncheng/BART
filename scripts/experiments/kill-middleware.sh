#!/bin/bash

source "experiments/common.sh"

if [[ $# -ne 1 ]]; then
  echo "Please specify whether to kill"
  exit
fi

kill_switch=$1

if [[ kill_switch -eq 1 ]]; then
  # stop controller
  ssh ${USER}@${MIDDILEWARE_CONTROLLER} "killall Controller"
fi

# start agents
node_index=0
for node in ${AGENTNODES[@]}; do
  echo $node " --- " $node_index
  ssh ${USER}@${node} "cat ~/node_agent_${node_index}.log | grep error "
  if [[ kill_switch -eq 1 ]]; then
    # stop agent
    echo "kill Agent on ${node}"
    ssh ${USER}@${node} "killall Agent"
  fi
  ((node_index+=1))
  echo -e "\n"
done
