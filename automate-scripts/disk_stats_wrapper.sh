#!/bin/bash

Usage () {
        echo "sh disk_stats_wrapper.sh  <device-type> <sleeptime>"
	echo "<device-type> : sdb(for HDD) | sdd(for SSD)"
        echo "<sleeptime> : Capture the statistics at this period of time interval"
        exit 0
}

if [ $# -ne 2 ]
then
        Usage
fi

sleepDurationSeconds=$2

while true;
do
	echo "======================"
	#Change accordingly for HDD(sdb) and SSD(sdd)
	cat /proc/diskstats | grep -i $1
	sleep $sleepDurationSeconds
done
