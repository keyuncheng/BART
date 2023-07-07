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
    # Settings: (6,2,2), (6,3,2), (6,3,3), (10,2,2)

    bash experiments/kill-middleware.sh 1

    # k_list=(6 6 6 10)
    # m_list=(2 3 3 2)
    # lambda_list=(2 2 3 2)
    k_list=(6)
    m_list=(3)
    lambda_list=(3)

    for index in "${!k_list[@]}"
    do
        k=${k_list[$index]}
        m=${m_list[$index]}
        lambda=${lambda_list[$index]}
        k_f=$(echo "$k*$lambda" | bc -l)

        block_size=67108864
        num_stripes=$(echo "scale=0; 1000/$lambda*$lambda" | bc)
        num_nodes=30

        sed -i "s/k_i = .*/k_i = $k/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/m_i = .*/m_i = $m/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/k_f = .*/k_f = $k_f/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/m_f = .*/m_f = $m/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_stripes = .*/num_stripes = $num_stripes/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_nodes = .*/num_nodes = $num_nodes/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/block_size = .*/block_size = $block_size/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_compute_workers = .*/num_compute_workers = 10/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_reloc_workers = .*/num_reloc_workers = 10/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini

        # Generate pre-transition information
        python3 gen_pre_stripes.py -config_filename ../conf/config.ini


        for approach in ${APPROACH_LIST[@]}; do
            # Modify config file
            sed -i "s/approach = .*/approach = $approach/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini

            # Initialize log file
            echo "" > ${MIDDLEWARE_HOME_PATH}/log/exp_b1/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log

            # Generate post-transition information
            python3 gen_post_stripes_meta.py -config_filename ../conf/config.ini >> ${MIDDLEWARE_HOME_PATH}/log/exp_b1/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log

            # Distribute blocks 
            python3 distribute_blocks.py -config_filename ../conf/config.ini 

            sleep 5
            
            # Execute 
            bash experiments/deploy-middleware.sh

            # Gather results
            trial=1
            while [ $trial -le 50 ]; do
                output=$(bash experiments/gather-time.sh)
                if [[ "$output" == *"min: -1"* ]]; then
                    echo "Results not ready ..."
                    ((trial+=1))
                    sleep 10
                else
                    last_line=$(echo "$output" | tail -1)
                    echo "${k}_${m}_${lambda}_${approach}: " $last_line >> ${MIDDLEWARE_HOME_PATH}/log/exp_b1/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log
                    break
                fi
                sleep 10
            done

            if [[ $trial -eq 50 ]]; then
                echo "${k}_${m}_${lambda}_${approach}: " $last_line >> ${MIDDLEWARE_HOME_PATH}/log/exp_b1/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log
            fi

            # Kill unfinished middleware
            bash experiments/kill-middleware.sh 1 
        done
    done
elif [[ exp_index -eq 2 ]]; then
    # Experiment B2 - Impact of Block Sizes
    # Settings: (6,3,3), (10,2,2)

    bash experiments/kill-middleware.sh 1

    # k_list=(6)
    # m_list=(3)
    # lambda_list=(3)
    k_list=(6 10)
    m_list=(3 2)
    lambda_list=(3 2)
    # block_size_list=(33554432)
    block_size_list=(33554432 67108864 134217728)

    for index in "${!k_list[@]}"
    do
        k=${k_list[$index]}
        m=${m_list[$index]}
        lambda=${lambda_list[$index]}
        k_f=$(echo "$k*$lambda" | bc -l)
        num_stripes=$(echo "scale=0; 1000/$lambda*$lambda" | bc)
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
        python3 gen_pre_stripes.py -config_filename ../conf/config.ini

        for approach in ${APPROACH_LIST[@]}; do
            # echo $pre_transition_output >> ${MIDDLEWARE_HOME_PATH}/log/exp_b2/load_${k}_${m}_${lambda}_${approach}_${num_stripes}.log

            # Modify config file
            sed -i "s/approach = .*/approach = $approach/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini

            # Generate post-transition information
            post_output=$(python3 gen_post_stripes_meta.py -config_filename ../conf/config.ini)
            if [[ $k -eq "10" && "$approach" == "BWPM" ]]; then
                bwpm_max_load=$(echo $post_output | sed 's/.*max_load: \([0-9]*\),.*/\1/')
                while [[ $bwpm_max_load -lt 52 ]]; do
                    python3 gen_pre_stripes.py -config_filename ../conf/config.ini
                    post_output=$(python3 gen_post_stripes_meta.py -config_filename ../conf/config.ini)
                    bwpm_max_load=$(echo $post_output | sed 's/.*max_load: \([0-9]*\),.*/\1/')
                done
            elif [[ $k -eq "6" && "$approach" == "BWPM" ]]; then
                bwpm_max_load=$(echo $post_output | sed 's/.*max_load: \([0-9]*\),.*/\1/')
                while [[ $bwpm_max_load -lt 65 ]]; do
                    python3 gen_pre_stripes.py -config_filename ../conf/config.ini
                    post_output=$(python3 gen_post_stripes_meta.py -config_filename ../conf/config.ini)
                    bwpm_max_load=$(echo $post_output | sed 's/.*max_load: \([0-9]*\),.*/\1/')
                done
            fi
            echo $post_output
            # echo $bwpm_max_load
            echo "------------"
            echo $post_output > ${MIDDLEWARE_HOME_PATH}/log/exp_b2/load_${k}_${m}_${lambda}_${approach}_${num_stripes}.log

            for block_size in ${block_size_list[@]}; do    
                # Modify config file
                sed -i "s/block_size = .*/block_size = $block_size/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini

                # Initialize log file
                echo "" > ${MIDDLEWARE_HOME_PATH}/log/exp_b2/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log

                # Distribute blocks 
                python3 distribute_blocks.py -config_filename ../conf/config.ini 

                sleep 30
                
                # Execute 
                bash experiments/deploy-middleware.sh

                # Gather results
                trial=1
                while [ $trial -le 50 ]; do
                    output=$(bash experiments/gather-time.sh)
                    avg_value=$(echo $output | awk -F 'avg: ' '{print $2}')
                    # avg_value=$(echo $output | sed 's/.*avg: \([0-9\.]*\).*/\1/')
                    if [ -z $avg_value ]; then
                        echo "Results not ready ..."
                        ((trial+=1))
                        sleep 10
                    else
                        last_line=$(echo "$output" | tail -1)
                        echo "${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}: " $last_line >> ${MIDDLEWARE_HOME_PATH}/log/exp_b2/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log
                        break
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
    # Settings: (6,3,3), (10,2,2)

    bash experiments/kill-middleware.sh 1

    k_list=(6 10)
    m_list=(3 2)
    lambda_list=(3 2)
    network_bandwidth_list=(1048576 2097152 5242880 10485760)

    for index in "${!k_list[@]}"
    do
        k=${k_list[$index]}
        m=${m_list[$index]}
        lambda=${lambda_list[$index]}
        num_stripes=$(echo "scale=0; 1000/$lambda*$lambda" | bc)
        k_f=$(echo "$k*$m" | bc -l)
        block_size=67108864
        num_nodes=30

        sed -i "s/k_i = .*/k_i = $k/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/m_i = .*/m_i = $m/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/k_f = .*/k_f = $k_f/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/m_f = .*/m_f = $m/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_stripes = .*/num_stripes = $num_stripes/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_nodes = .*/num_nodes = $num_nodes/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/block_size = .*/block_size = $block_size/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_compute_workers = .*/num_compute_workers = 10/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini
        sed -i "s/num_reloc_workers = .*/num_reloc_workers = 10/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini

        # Generate pre-transition information
        python3 gen_pre_stripes.py -config_filename ../conf/config.ini

        for approach in ${APPROACH_LIST[@]}; do
            sed -i "s/approach = .*/approach = $approach/" ${MIDDLEWARE_HOME_PATH}/conf/config.ini

            # Generate post-transition information
            post_output=$(python3 gen_post_stripes_meta.py -config_filename ../conf/config.ini)
            if [[ $k -eq "10" && "$approach" == "BWPM" ]]; then
                bwpm_max_load=$(echo $post_output | sed 's/.*max_load: \([0-9]*\),.*/\1/')
                while [[ $bwpm_max_load -lt 52 ]]; do
                    python3 gen_pre_stripes.py -config_filename ../conf/config.ini
                    post_output=$(python3 gen_post_stripes_meta.py -config_filename ../conf/config.ini)
                    bwpm_max_load=$(echo $post_output | sed 's/.*max_load: \([0-9]*\),.*/\1/')
                done
            elif [[ $k -eq "6" && "$approach" == "BWPM" ]]; then
                bwpm_max_load=$(echo $post_output | sed 's/.*max_load: \([0-9]*\),.*/\1/')
                while [[ $bwpm_max_load -lt 65 ]]; do
                    python3 gen_pre_stripes.py -config_filename ../conf/config.ini
                    post_output=$(python3 gen_post_stripes_meta.py -config_filename ../conf/config.ini)
                    bwpm_max_load=$(echo $post_output | sed 's/.*max_load: \([0-9]*\),.*/\1/')
                done
            fi

            echo $post_output
            # echo $bwpm_max_load
            echo "------------"
            echo $post_output > ${MIDDLEWARE_HOME_PATH}/log/exp_b3/load_${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}.log
    
            for bandwidth in ${network_bandwidth_list[@]}; do
                # Set bandwidth
                bash experiments/cancel-wondershaper.sh
                bash experiments/setup-wondershaper.sh ${bandwidth}

                # Initialize log
                echo "" > ${MIDDLEWARE_HOME_PATH}/log/exp_b3/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}_${bandwidth}.log

                sleep 10

                # Distribute blocks 
                python3 distribute_blocks.py -config_filename ../conf/config.ini 
                
                # Execute 
                bash experiments/deploy-middleware.sh

                # Gather results
                trial=1
                while [ $trial -le 50 ]; do
                    output=$(bash experiments/gather-time.sh)
                    avg_value=$(echo $output | awk -F 'avg: ' '{print $2}')
                    # avg_value=$(echo $output | sed 's/.*avg: \([0-9\.]*\).*/\1/')
                    if [ -z $avg_value ]; then
                        grep_error=$(bash experiments/kill-middleware.sh 0 | grep error | wc -l)
                        if [[ $grep_error -lt 0 ]]; then
                            break;
                        fi
                        echo "Results not ready ..."
                        ((trial+=1))
                        sleep 10
                    else
                        last_line=$(echo "$output" | tail -1)
                        echo "${k}_${m}_${lambda}_${approach}_${bandwidth}: " $last_line >> ${MIDDLEWARE_HOME_PATH}/log/exp_b3/${k}_${m}_${lambda}_${approach}_${block_size}_${num_stripes}_${bandwidth}.log
                        break
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
