# HDFS Integration

## Preparation
1. Download `hadoop-3.3.4-src.tar.gz` and decompress it to local.
2. Follow the building instructions provided by Hadoop and prepare the required packages.
3. Modify the Hadoop src code path in `install.sh`.

## Install and Deploy
1. Install BART-integration with `bash install.sh`.
2. Deploy Hadoop in the cluster with `bash deploy-hadoop.sh`.

## BART Experiments

1. Write $N$ stripes to HDFS with scripts.
2. Retrieve pre-transitioning stripe placement with `python3 gen_pre_placement.py -config_filename [config_file_path]`.
3. [Middleware Side] Generate transitioning solution and execute transition.
4. Obtain post-transitioning stripe placement with `python3 recover_post_placement.py`.
Evaluate post-transitioning stripe with `hadoop fs -get [filename].`