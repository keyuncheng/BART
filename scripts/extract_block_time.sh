#!/bin/bash

source "experiments/common.sh"

# if [[ $# -ne 1 ]]; then
#   echo "Please specify the bandwidth"
#   exit
# fi

# bandwidth=$1

node_index=0
for node in ${AGENTNODES[@]}; do
  echo $node " --- " $node_index
  scp ${USER}@${node}:~/node_agent_${node_index}.log ${MIDDLEWARE_HOME_PATH}/scripts/tmp_logs/
  ((node_index+=1))
done

cd ${MIDDLEWARE_HOME_PATH}/scripts/tmp_logs/

for log_index in {0..29}; do
  sendstarttime=0.0
  sendendtime=0.0
  recvstarttime=0.0
  recvendtime=0.0

  while read line; do
    if [[ $line =~ recvBlock::start\ ([0-9]+\.[0-9]+) ]]; then
      timestamp="${BASH_REMATCH[1]}"
      if [[ $sendstarttime == 0.0 ]]; then
        sendstarttime=$timestamp
      fi
    fi
    if [[ $line =~ sendBlock::start\ ([0-9]+\.[0-9]+) ]]; then
      timestamp="${BASH_REMATCH[1]}"
      if [[ $recvstarttime == 0.0 ]]; then
        recvstarttime=$timestamp
      fi
    fi
    if [[ $line =~ sendBlock::end\ ([0-9]+\.[0-9]+) ]]; then
      sendendtime="${BASH_REMATCH[1]}"
    fi
    if [[ $line =~ recvBlock::end\ ([0-9]+\.[0-9]+) ]]; then
      recvendtime="${BASH_REMATCH[1]}"
    fi
  done < "node_agent_${log_index}.log"

  diffsend=$(echo "$sendendtime - $sendstarttime" | bc)
  diffrecv=$(echo "$recvendtime - $recvstarttime" | bc)

  echo $log_index $diffsend $diffrecv

  ((log_index+=1))
done

