#!/bin/bash

source "experiments/common.sh"

if [[ $# -ne 1 ]]; then
  echo "Please specify the full path of file"
  exit
fi

target_file=$1

for node in ${AGENTNODES[@]}; do
  scp -r ${target_file} ${USER}@${node}:${target_file}
done
