#!/bin/bash

source "experiments/common.sh"

if [[ $# -ne 1 ]]; then
  echo "Please specify the experiment to run"
  echo "    Usage: bash exec_testbed.sh {experiment_index}"
  exit
fi

exp_index=$1

if [[ exp_index -eq 1 ]]; then
    # Experiment B1 - Transitioning Time Reduction 
    # Settings: (4,2,2), (6,2,2), (6,3,2), (6,3,3)

    # Step B1-1: kill running middleware
    bash experiments/kill-middleware.sh 1

    # Step B1-2: prepare config
    k_list=(4 6 6 6)
    m_list=(2 2 3 3)
    lambda_list=(2 2 2 3)

    for index in "${!k_list[@]}"
    do
        k=${k_list[$index]}
        m=${m_list[$index]}
        lambda=${lambda_list[$index]}
        k_f=$(echo "$k*$lambda" | bc -l)
        block_size=67108864
        num_stripes=$(echo "scale=0; 1000/$lambda*$lambda" | bc)
        num_nodes=30

        for approach in ${APPROACH_LIST[@]}; do
            sed -i "s/k_i = .*/k_i = $k/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
            sed -i "s/m_i = .*/m_i = $m/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
            sed -i "s/k_f = .*/k_f = $k_f/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
            sed -i "s/m_f = .*/m_f = $m/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
            sed -i "s/num_stripes = .*/num_stripes = $num_stripes/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
            sed -i "s/num_nodes = .*/num_nodes = $num_nodes/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
            sed -i "s/approach = .*/approach = $approach/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
            sed -i "s/block_size = .*/block_size = $block_size/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini

            if [ "$result" == "RDPM" ]; then
                sed -i "s/num_compute_workers = .*/num_compute_workers = 1/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                sed -i "s/num_reloc_workers = .*/num_reloc_workers = 1/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
            else
                sed -i "s/num_compute_workers = .*/num_compute_workers = 20/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                sed -i "s/num_reloc_workers = .*/num_reloc_workers = 20/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
            fi
            # cp ${MIDDLEWARE_HOME_PATH}/conf/config.ini ./config_${k}_${m}_${lambda}_${approach}.ini

            # Initialize log file
            echo "" > ${MIDDLEWARE_HOME_PATH}/log/exp_b1/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log

            # Generate pre-transition information
            python3 gen_pre_stripes.py -config_filename ../conf/config.ini >> ${MIDDLEWARE_HOME_PATH}/log/exp_b1/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log

            # Generate post-transition information
            python3 gen_post_stripes_meta.py -config_filename ../conf/config.ini >> ${MIDDLEWARE_HOME_PATH}/log/exp_b1/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log

            # Distribute blocks 
            python3 distribute_blocks.py -config_filename ../conf/config.ini 
            
            # Execute 
            bash experiments/deploy-middleware.sh

            # Gather results
            trial=1
            while [ $trial -le 10 ]; do
                output=$(bash experiments/gather-time.sh)
                if [[ "$output" == *"min: -1"* ]]; then
                    echo "Results not ready ..."
                    sleep 5
                else
                    last_line=$(echo "$output" | tail -1)
                    echo "${k}_${m}_${lambda}_${approach}: " $last_line >> ${MIDDLEWARE_HOME_PATH}/log/exp_b1/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log
                fi
                sleep 10
            done

            # Kill unfinished middleware
            bash experiments/kill-middleware.sh 1 
        done
    done
elif [[ exp_index -eq 2 ]]; then
    # Experiment B2 - Impact of Block Sizes
    # Settings: (4,2,2), (6,3,3)

    bash experiments/kill-middleware.sh 1

    k_list=(4 6)
    m_list=(2 3)
    lambda_list=(2 3)

    for index in "${!k_list[@]}"
    do
        k=${k_list[$index]}
        m=${m_list[$index]}
        lambda=${lambda_list[$index]}
        k_f=$(echo "$k*$lambda" | bc -l)
        num_stripes=$(echo "scale=0; 1000/$lambda*$lambda" | bc)
        num_nodes=30

        block_size_list=(33554432 67108864 134217728)

        for approach in ${APPROACH_LIST[@]}; do
            for block_size in ${block_size_list[@]}; do    
                sed -i "s/k_i = .*/k_i = $k/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                sed -i "s/m_i = .*/m_i = $m/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                sed -i "s/k_f = .*/k_f = $k_f/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                sed -i "s/m_f = .*/m_f = $m/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                sed -i "s/num_stripes = .*/num_stripes = $num_stripes/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                sed -i "s/num_nodes = .*/num_nodes = $num_nodes/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                sed -i "s/approach = .*/approach = $approach/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                sed -i "s/block_size = .*/block_size = $block_size/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini

                if [ "$result" == "RDPM" ]; then
                    sed -i "s/num_compute_workers = .*/num_compute_workers = 1/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                    sed -i "s/num_reloc_workers = .*/num_reloc_workers = 1/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                else
                    sed -i "s/num_compute_workers = .*/num_compute_workers = 20/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                    sed -i "s/num_reloc_workers = .*/num_reloc_workers = 20/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                fi
                # cp ${MIDDLEWARE_HOME_PATH}/conf/config.ini ./config_${k}_${m}_${lambda}_${approach}.ini

                # Initialize log file
                echo "" > ${MIDDLEWARE_HOME_PATH}/log/exp_b2/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log

                # Generate pre-transition information
                python3 gen_pre_stripes.py -config_filename ../conf/config.ini >> ${MIDDLEWARE_HOME_PATH}/log/exp_b2/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log

                # Generate post-transition information
                python3 gen_post_stripes_meta.py -config_filename ../conf/config.ini >> ${MIDDLEWARE_HOME_PATH}/log/exp_b2/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log

                # Distribute blocks 
                python3 distribute_blocks.py -config_filename ../conf/config.ini 
                
                # Execute 
                bash experiments/deploy-middleware.sh

                # Gather results
                trial=1
                while [ $trial -le 10 ]; do
                    output=$(bash experiments/gather-time.sh)
                    if [[ "$output" == *"min: -1"* ]]; then
                        echo "Results not ready ..."
                        sleep 5
                    else
                        last_line=$(echo "$output" | tail -1)
                        echo "${k}_${m}_${lambda}_${approach}_${block_size}: " $last_line >> exp_b2_${k}_${m}_${lambda}_${approach}_${block_size}.log
                    fi
                    sleep 10
                done

                # Kill unfinished middleware
                bash experiments/kill-middleware.sh 1 
                echo "----------" >> ${MIDDLEWARE_HOME_PATH}/log/exp_b2/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log
            done
        done
    done
elif [[ exp_index -eq 3 ]]; then
    # Experiment B3 - Impact of Network Bandwidth
    # Settings: (4,2,2), (6,3,3)

    bash experiments/kill-middleware.sh 1

    k_list=(4 6)
    m_list=(2 3)
    lambda_list=(2 3)

    network_bandwidth_list=(1048576 2097152 5242880 10485760)
    for bandwidth in ${network_bandwidth_list[@]}; do
        # Set bandwidth (TODO: solve the sudo password)
        sed -i "s/BANDWIDTH=.*/BANDWIDTH = $bandwidth/" ${MIDDLEWARE_HOME_PATH}/scripts/experiments/common.sh
        # bash experiments/setup-wondershaper.sh

        for index in "${!k_list[@]}"
        do
            k=${k_list[$index]}
            m=${m_list[$index]}
            lambda=${lambda_list[$index]}
            num_stripes=$(echo "scale=0; 1000/$lambda*$lambda" | bc)
            k_f=$(echo "$k*$m" | bc -l)
            block_size=67108864
            num_nodes=30

            for approach in ${APPROACH_LIST[@]}; do
                sed -i "s/k_i = .*/k_i = $k/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                sed -i "s/m_i = .*/m_i = $m/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                sed -i "s/k_f = .*/k_f = $k_f/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                sed -i "s/m_f = .*/m_f = $m/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                sed -i "s/num_stripes = .*/num_stripes = $num_stripes/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                sed -i "s/num_nodes = .*/num_nodes = $num_nodes/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                sed -i "s/approach = .*/approach = $approach/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                sed -i "s/block_size = .*/block_size = $block_size/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini

                if [ "$result" == "RDPM" ]; then
                    sed -i "s/num_compute_workers = .*/num_compute_workers = 1/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                    sed -i "s/num_reloc_workers = .*/num_reloc_workers = 1/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                else
                    sed -i "s/num_compute_workers = .*/num_compute_workers = 20/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                    sed -i "s/num_reloc_workers = .*/num_reloc_workers = 20/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
                fi
                # cp ${MIDDLEWARE_HOME_PATH}/conf/config.ini ./config_${k}_${m}_${lambda}_${approach}.ini

                # Initialize log
                echo "" > ${MIDDLEWARE_HOME_PATH}/log/exp_b3/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}_${bandwidth}.log

                # Generate pre-transition information
                python3 gen_pre_stripes.py -config_filename ../conf/config.ini >> ${MIDDLEWARE_HOME_PATH}/log/exp_b3/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}_${bandwidth}.log

                # Generate post-transition information
                python3 gen_post_stripes_meta.py -config_filename ../conf/config.ini >> ${MIDDLEWARE_HOME_PATH}/log/exp_b3/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}_${bandwidth}.log

                # Distribute blocks 
                python3 distribute_blocks.py -config_filename ../conf/config.ini 
                
                # Execute 
                bash experiments/deploy-middleware.sh

                # Gather results
                trial=1
                while [ $trial -le 10 ]; do
                    output=$(bash experiments/gather-time.sh)
                    if [[ "$output" == *"min: -1"* ]]; then
                        echo "Results not ready ..."
                        sleep 5
                    else
                        last_line=$(echo "$output" | tail -1)
                        echo "${k}_${m}_${lambda}_${approach}_${bandwidth}: " $last_line >> ${MIDDLEWARE_HOME_PATH}/log/exp_b3/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}_${bandwidth}.log
                    fi
                    sleep 10
                done

                # Kill unfinished middleware
                bash experiments/kill-middleware.sh 1 

                echo "----------" >> ${MIDDLEWARE_HOME_PATH}/log/exp_b3/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}_${bandwidth}.log
            done
        done
    done
else
    echo "Unexpected experiment index!"
fi