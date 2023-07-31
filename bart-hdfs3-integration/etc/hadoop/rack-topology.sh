#!/bin/bash
# Adjust/Add the property "net.topology.script.file.name"
# to core-site.xml with the "absolute" path the this
# file. ENSURE the file is "executable".

# Supply appropriate rack prefix
RACK_PREFIX=default

# To test, supply a hostname as script input:
if [ $# -gt 0 ]; then

  CTL_FILE=${CTL_FILE:-"rack-topology.data"}

  HADOOP_CONF=${HADOOP_CONF:-"/home/bart/hadoop-3.3.4/etc/hadoop"}

  if [ ! -f ${HADOOP_CONF}/${CTL_FILE} ]; then
    echo -n "/$RACK_PREFIX/rack "
    exit 0
  fi

  while [ $# -gt 0 ]; do
    nodeArg=$1
    exec <${HADOOP_CONF}/${CTL_FILE}
    result=""
    while read line; do
      ar=($line)
      if [ "${ar[0]}" = "$nodeArg" ]; then
        result="${ar[1]}"
      fi
    done
    shift
    if [ -z "$result" ]; then
      echo -n "/$RACK_PREFIX/rack "
    else
      echo -n "/$RACK_PREFIX/rack_$result "
    fi
  done

else
  echo -n "/$RACK_PREFIX/rack "
fi

