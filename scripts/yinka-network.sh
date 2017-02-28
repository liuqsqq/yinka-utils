#!/bin/bash -e

OPTION=$1

function netstat_check () {
	cnt=0
	while [ $cnt -le 10 ]
	do
   		ping -c2 www.yinka.co  > /dev/null 2>&1
   		if [ $? -eq  0 ]; then
       	break
    	else
       	cnt=$(($cnt+1))
    	fi
	done
	if [ $cnt -gt 10 ]; then
	echo "try aggin"
    	network_change
	fi
}

function network_change () {
	if [ $OPTION == "4g" ]; then
		ifconfig wlan0 down
		ifconfig enx0c5b8f279a64 up
		dhclient enx0c5b8f279a64
	elif [ $OPTION == "wifi" ]; then
		ifconfig enx0c5b8f279a64 down
		ifconfig wlan0 up
		service wicd restart
	else
		echo "none support network type"
		exit 1
	fi
}

network_change
sleep 10
netstat_check

exit 0
