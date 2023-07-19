#!/bin/bash

HADOOP_SRC_DIR=/home/bart/hadoop-3.3.4-src

# Step 0: sync Balanced Transitioning modifications to hadoop source code
cp src/HdfsClientConfigKeys.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/java/org/apache/hadoop/hdfs/client/
cp src/BlockReaderRemote.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/java/org/apache/hadoop/hdfs/client/impl/
cp src/DFSClient.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/java/org/apache/hadoop/hdfs/
cp src/DFSInputStream.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/java/org/apache/hadoop/hdfs/
cp src/DFSStripedInputStream.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/java/org/apache/hadoop/hdfs/
cp src/DatanodeID.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/java/org/apache/hadoop/hdfs/protocol/
cp src/DatanodeInfo.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/java/org/apache/hadoop/hdfs/protocol/
cp src/DatanodeInfoWithStorage.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/java/org/apache/hadoop/hdfs/protocol/
cp src/ExtendedBlock.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/java/org/apache/hadoop/hdfs/protocol/
cp src/LocatedBlock.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/java/org/apache/hadoop/hdfs/protocol/
cp src/LocatedBlocks.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/java/org/apache/hadoop/hdfs/protocol/
cp src/LocatedStripedBlock.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/java/org/apache/hadoop/hdfs/protocol/
cp src/PBHelperClient.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/java/org/apache/hadoop/hdfs/protocolPB/
cp src/StripeReader.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/java/org/apache/hadoop/hdfs/
cp src/TransitionedStripeReader.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/java/org/apache/hadoop/hdfs/
cp src/StripedBlockUtil.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/java/org/apache/hadoop/hdfs/util/
cp src/hdfs.proto $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/proto/
cp src/DFSConfigKeys.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/
cp src/BlockInfo.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/blockmanagement/
cp src/BlockInfoStriped.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/blockmanagement/
cp src/DatanodeStorageInfo.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/blockmanagement/
cp src/BlockSender.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/datanode/
cp src/DataXceiver.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/datanode/
cp src/FileIoProvider.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/datanode/
cp src/FsDatasetSpi.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/datanode/fsdataset/
cp src/FsDatasetImpl.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/datanode/fsdataset/impl/
cp src/FsVolumeImpl.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/datanode/fsdataset/impl/
cp src/FsVolumeImplBuilder.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/datanode/fsdataset/impl/
cp src/FSNamesystem.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/namenode/
cp src/NameNodeRpcServer.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/namenode/
cp src/ExternalDatasetImpl.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/test/java/org/apache/hadoop/hdfs/server/datanode/extdataset/
cp src/SimulatedFSDataset.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/test/java/org/apache/hadoop/hdfs/server/datanode/
cp src/pom.xml $HADOOP_SRC_DIR/

# Step 1: Build hadoop source code

cd $HADOOP_SRC_DIR
mvn package -Pdist,native -DskipTests -Dtar -Dmaven.javadoc.skip=true

# Step 2: Deploy hadoop
cp $HADOOP_SRC_DIR/hadoop-dist/target/hadoop-3.3.4.tar.gz /home/bart/
