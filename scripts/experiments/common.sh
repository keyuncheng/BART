#!/bin/bash

USER="bart"
HOME_DIR="/home/${USER}"

## Agent Nodes
AGENTNODES=("agent00" "agent01" "agent02" "agent03" "agent04" "agent05" "agent06" "agent07" "agent08" "agent09" "agent10" "agent11" "agent12" "agent13" "agent14" "agent15" "agent16" "agent17" "agent18" "agent19" "agent20" "agent21" "agent22" "agent23" "agent24" "agent25" "agent26" "agent27" "agent28" "agent29")

## Middleware Configs
MIDDILEWARE_CONTROLLER="controller"
MIDDLEWARE_HOME_PATH="${HOME_DIR}/BalancedConversion/"

## Experiment Methods
APPROACH_LIST=("BWPM" "BTPM" "RDPM")