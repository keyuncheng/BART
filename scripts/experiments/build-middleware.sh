#!/bin/bash

source "./common.sh"

nohup sh -c 'cd ${MIDDLEWARE_HOME_PATH}/build && rm -r * && cmake .. && make' > output.log 2>&1 & 

# build middleware
for node in ${AGENTNODES[@]}; do
  ssh ${USER}@${node} "nohup sh -c 'cd ${MIDDLEWARE_HOME_PATH}/build && rm -r * && cmake .. && make' > output.log 2>&1 &"
done
