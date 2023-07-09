#!/bin/bash

USER="bart"
HOME_DIR="/home/${USER}"

DATANODES=("agent00" "agent01" "agent02" "agent03" "agent04" "agent05" "agent06" "agent07" "agent08" "agent09" "agent10" "agent11" "agent12" "agent13" "agent14" "agent15" "agent16" "agent17" "agent18" "agent19" "agent20" "agent21" "agent22" "agent23" "agent24" "agent25" "agent26" "agent27" "agent28" "agent29")
AGENTNODES=("agent00" "agent01" "agent02" "agent03" "agent04" "agent05" "agent06" "agent07" "agent08" "agent09" "agent10" "agent11" "agent12" "agent13" "agent14" "agent15" "agent16" "agent17" "agent18" "agent19" "agent20" "agent21" "agent22" "agent23" "agent24" "agent25" "agent26" "agent27" "agent28" "agent29")


## middleware configs
MIDDILEWARE_CONTROLLER="controller"
MIDDLEWARE_HOME_PATH="${HOME_DIR}/BalancedConversion/"

## Placement Filename
ORIGINAL_PATTERN_FILE="${HOME_DIR}/hadoop-balanced-conversion/hadoop-transition-scripts/placementandmapping/"
HDFS_PATTERN_FILE="${HOME_DIR}/BalancedConversion/metadata/"

## Experiment Methods
# APPROACH_LIST=("RDPM")
# APPROACH_LIST=("BWPM" "BTPM")
APPROACH_LIST=("BWPM" "BTPM" "RDPM")
# APPROACH_LIST=("BTPM")

## Network Bandwidth
BANDWIDTH=1048576