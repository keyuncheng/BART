#!/bin/bash

source "experiments/common.sh"

if [[ $# -ne 1 ]]; then
  echo "Please specify the experiment to run"
  echo "    Usage: bash generate_placements.sh {6|10}"
  exit
fi

exp_k=$1

method_list=("BWPM" "BTPM")

if [[ exp_k -eq 6 ]]; then
    mkdir -p ${MIDDLEWARE_HOME_PATH}/scripts/placements/633
    current_pre_iteration=0
    while [ $current_pre_iteration -lt 10 ]; do
        target_directory=${MIDDLEWARE_HOME_PATH}/scripts/placements/633/round$current_pre_iteration
        
        mkdir -p $target_directory

        sed -i "s/k_i = .*/k_i = 6/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/m_i = .*/m_i = 3/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/k_f = .*/k_f = 18/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/m_f = .*/m_f = 3/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_stripes = .*/num_stripes = 1200/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_nodes = .*/num_nodes = 30/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/block_size = .*/block_size = 67108864/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_compute_workers = .*/num_compute_workers = 10/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_reloc_workers = .*/num_reloc_workers = 10/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini    

        # Generate pre-transition information
        python3 gen_pre_stripes.py -config_filename ../conf/config.ini 
        cp ${MIDDLEWARE_HOME_PATH}/metadata/pre_block_mapping $target_directory
        cp ${MIDDLEWARE_HOME_PATH}/metadata/pre_placement $target_directory

        current_post_iteration=0
        while [ $current_post_iteration -lt 1 ]; do
            for method in ${method_list[@]}; do
                sed -i "s/approach = .*/approach = $method/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                
                mkdir -p $target_directory/$method/round$current_post_iteration

                python3 gen_post_stripes_meta.py -config_filename ../conf/config.ini >> $target_directory/$method/post_placement_results.log
                
                cp ${MIDDLEWARE_HOME_PATH}/metadata/post_block_mapping $target_directory/$method/round$current_post_iteration/
                cp ${MIDDLEWARE_HOME_PATH}/metadata/post_placement $target_directory/$method/round$current_post_iteration/
                cp ${MIDDLEWARE_HOME_PATH}/metadata/sg_meta $target_directory/$method/round$current_post_iteration/
            done
            ((current_post_iteration+=1)) 
        done
        ((current_pre_iteration+=1))
    done

elif [[ exp_k -eq 10 ]]; then
    mkdir -p ${MIDDLEWARE_HOME_PATH}/scripts/placements/1022
    current_pre_iteration=0
    while [ $current_pre_iteration -lt 10 ]; do
        target_directory=${MIDDLEWARE_HOME_PATH}/scripts/placements/1022/round$current_pre_iteration
        
        mkdir -p $target_directory

        sed -i "s/k_i = .*/k_i = 10/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/m_i = .*/m_i = 2/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/k_f = .*/k_f = 20/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/m_f = .*/m_f = 2/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_stripes = .*/num_stripes = 1200/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_nodes = .*/num_nodes = 30/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/block_size = .*/block_size = 67108864/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_compute_workers = .*/num_compute_workers = 10/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_reloc_workers = .*/num_reloc_workers = 10/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini    

        # Generate pre-transition information
        python3 gen_pre_stripes.py -config_filename ../conf/config.ini 
        cp ${MIDDLEWARE_HOME_PATH}/metadata/pre_block_mapping $target_directory
        cp ${MIDDLEWARE_HOME_PATH}/metadata/pre_placement $target_directory

        current_post_iteration=0
        while [ $current_post_iteration -lt 1 ]; do
            for method in ${method_list[@]}; do
                sed -i "s/approach = .*/approach = $method/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                
                mkdir -p $target_directory/$method/round$current_post_iteration

                python3 gen_post_stripes_meta.py -config_filename ../conf/config.ini >> $target_directory/$method/post_placement_results.log
                
                cp ${MIDDLEWARE_HOME_PATH}/metadata/post_block_mapping $target_directory/$method/round$current_post_iteration/
                cp ${MIDDLEWARE_HOME_PATH}/metadata/post_placement $target_directory/$method/round$current_post_iteration/
                cp ${MIDDLEWARE_HOME_PATH}/metadata/sg_meta $target_directory/$method/round$current_post_iteration/
            done
            ((current_post_iteration+=1)) 
        done
        ((current_pre_iteration+=1))
    done
fi