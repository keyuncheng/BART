#!/bin/bash

source "experiments/common.sh"

if [[ $# -ne 3 ]]; then
  echo "Please specify the experiment to run"
  echo "    Usage: bash exec_testbed_exp2.sh {num_of_pre} {num_of_post} {flag}"
  exit
fi

num_of_pre=$1
num_of_post=$2
exp_flag=$3

#### TEMP CODE ####
while true; do
    exec_count=$(ps aux | grep exp3 | wc -l)
    if [ $exec_count -gt 1 ]; then
        echo "Found $exec_count exp3 processes. Sleeping for 20 minutes..."
        sleep 1200
    else
        echo "Found $exec_count exp3 processes. Breaking loop."
        break
    fi
done
#### TEMP CODE ####


# Experiment B2 - Impact of Block Sizes
# Settings: (6,3,3), (10,2,2)

mkdir -p ${MIDDLEWARE_HOME_PATH}/log/exp_b2_${exp_flag}

bash experiments/kill-middleware.sh 1

bash experiments/cancel-wondershaper.sh
bash experiments/setup-wondershaper.sh 1048576


k_list=(6 10)
m_list=(3 2)
lambda_list=(3 2)
# network_bandwidth_list=(10485760)
block_size_list=(33554432 67108864 134217728)

current_iteration=0
while [ $current_iteration -lt $num_of_pre ]; do
    for index in "${!k_list[@]}"
    do
        k=${k_list[$index]}
        m=${m_list[$index]}
        lambda=${lambda_list[$index]}
        k_f=$(echo "$k*$lambda" | bc -l)
        num_stripes=1200
        num_nodes=30

        sed -i "s/k_i = .*/k_i = $k/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/m_i = .*/m_i = $m/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/k_f = .*/k_f = $k_f/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/m_f = .*/m_f = $m/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_stripes = .*/num_stripes = $num_stripes/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_nodes = .*/num_nodes = $num_nodes/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_compute_workers = .*/num_compute_workers = 10/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_reloc_workers = .*/num_reloc_workers = 10/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini


        # Generate pre-transition information
        placement_src=""
        if [[ $k -eq 6 ]]; then
            placement_src=${MIDDLEWARE_HOME_PATH}/scripts/placements/633/round$current_iteration
        else
            placement_src=${MIDDLEWARE_HOME_PATH}/scripts/placements/1022/round$current_iteration
        fi
        cp $placement_src/pre_block_mapping ${MIDDLEWARE_HOME_PATH}/metadata/
        cp $placement_src/pre_placement ${MIDDLEWARE_HOME_PATH}/metadata/

        for approach in ${APPROACH_LIST[@]}; do
            # Modify config file
            sed -i "s/approach = .*/approach = $approach/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini

            for block_size in ${block_size_list[@]}; do  
                sed -i "s/block_size = .*/block_size = $block_size/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini

                # Initialize log file
                if [[ $current_iteration -eq 0 ]]; then
                    # Initialize log file
                    echo "" > ${MIDDLEWARE_HOME_PATH}/log/exp_b2_${exp_flag}/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log
                fi

                echo "~~~~~~~~~~~~~"
                echo "$num_of_post"
                echo "~~~~~~~~~~~~~"
                current_post_iteration=0
                while [ $current_post_iteration -lt $num_of_post ]; do

                    # Generate post-transition information
                    post_placement_src=""
                    if [[ $k -eq 6 ]]; then
                        post_placement_src=${MIDDLEWARE_HOME_PATH}/scripts/placements/633/round$current_iteration/$approach/round$current_post_iteration
                    else
                        post_placement_src=${MIDDLEWARE_HOME_PATH}/scripts/placements/1022/round$current_iteration/$approach/round$current_post_iteration
                    fi
                    cp $post_placement_src/post_block_mapping ${MIDDLEWARE_HOME_PATH}/metadata/
                    cp $post_placement_src/post_placement ${MIDDLEWARE_HOME_PATH}/metadata/
                    cp $post_placement_src/sg_meta ${MIDDLEWARE_HOME_PATH}/metadata/

                    error_trial=0
                    while [ $error_trial -le 10 ]; do
                
                        # Distribute blocks 
                        python3 distribute_blocks.py -config_filename ../conf/config.ini 
                    
                        sleep 10

                        # Execute 
                        bash experiments/deploy-middleware.sh

                        # Gather results
                        trial=1
                        while [ $trial -le 20 ]; do
                            output=$(bash experiments/gather-time.sh)
                            avg_value=$(echo $output | awk -F 'avg: ' '{print $2}')
                            # avg_value=$(echo $output | sed 's/.*avg: \([0-9\.]*\).*/\1/')
                            if [ -z $avg_value ]; then
                                grep_error=$(bash experiments/kill-middleware.sh 0 | grep error | wc -l)
                                if [[ $grep_error -gt 0 ]]; then
                                    echo "Execution error ..."
                                    break # break the loop for time check, restart from gen_post
                                fi
                                echo "Results not ready ..."
                                ((trial+=1))
                                sleep 10
                            else
                                last_line=$(echo "$output" | tail -1)
                                echo "${k}_${m}_${lambda}_${approach}_${block_size}: " $last_line >> ${MIDDLEWARE_HOME_PATH}/log/exp_b2_${exp_flag}/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log
                                break
                            fi
                            sleep 10
                        done    # end of check results iteration

                        if [[ $grep_error -ne 0 ]]; then
                            ((error_trial+=1))
                            bash experiments/kill-middleware.sh 1 
                            bash experiments/kill-middleware.sh 1 
                            sleep 10
                            # echo "${k}_${m}_${lambda}_${approach}: " $last_line >> ${MIDDLEWARE_HOME_PATH}/log/exp_b2_${exp_flag}/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log
                        else
                            bash experiments/kill-middleware.sh 1
                            break 
                        fi
                    done    # end of error trial iteration

                    ((current_post_iteration+=1))

                    if [[ $error_trial -gt 9 && $grep_error -ne 0 ]]; then
                        echo "Too many errors!" >> ${MIDDLEWARE_HOME_PATH}/log/exp_b2_${exp_flag}/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log
                        exit -1
                    fi
                    sleep 10
                done    # end of post mapping iteration
            done    # end of block size iteration
        done    # end of approach iteration
    done    # end of coding parameter iteration
    ((current_iteration+=1))
done    # end of pre-mapping iteration
