#!/bin/bash

source "experiments/common.sh"

# modify config and sync
num_nodes=20

num_nodes_provided=${#AGENTNODES[@]}
if [[ $num_nodes -ne $num_nodes_provided ]]; then
  echo $num_nodes_provided
  echo $num_nodes
  echo "error in node list provided "
  exit
fi


time_sum=0
time_max=0
time_min=-1

# start agents
node_index=0
for node in ${AGENTNODES[@]}; do
  echo "Time from $node"
  output=$(ssh ${USER}@${node} "cat ~/node_agent.log | grep time")
  echo $output
  iter_time=${output##*: }
  iter_time=${iter_time% ms}

  if (( $(echo "$iter_time > -1" | bc -l) )); then
    time_min=$iter_time;
  elif (( $(echo "$iter_time > $time_min" | bc -l) )); then
    time_min=$iter_time;
  fi

  if (( $(echo "$iter_time > $time_max" | bc -l) )); then
    time_max=$iter_time
  fi

  time_sum=$(echo "scale=6; $time_sum + $iter_time" | bc)
done

echo "Statistics"
time_avg=$(echo "scale=6; $time_sum / $num_nodes" | bc)

echo "max: $time_max; min: $time_min; avg: $time_avg"
