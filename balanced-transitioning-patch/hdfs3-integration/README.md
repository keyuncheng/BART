# HDFS Integration

## Preparation
1. Download `hadoop-3.3.4-src.tar.gz` and decompress it to local.
2. Follow the building instructions provided by Hadoop and prepare the required packages.
3. Modify the Hadoop src code path in `install.sh`.

## Install and Deploy
1. Install and deploy BART-integration with `bash install.sh`.
<!-- 2. Deploy Hadoop in the cluster with `bash deploy-hadoop.sh` -->

## HDFS configuration
1. Add `dfs.namenode.external.metadata.path` to hdfs-site.xml, and make sure its value is the same as the `metadata_file_path` in ../../conf/config.ini.