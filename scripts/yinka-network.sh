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
function open_wicd_gtk () {
	wicd-gtk&
	sleep 1
	killall -9 devilspie
	devilspie&	
}
function network_change () {
	if [ $OPTION == "4g" ]; then
		if ifconfig -a  |grep enx0c5b8f279a64  >/dev/null ;then
			service wicd stop
			ifconfig wlan0 down

			ifconfig enx0c5b8f279a64 up
			dhclient enx0c5b8f279a64
		else
			exit 1
		fi
	elif [ $OPTION == "wifi" ]; then
		if ifconfig -a |grep enx0c5b8f279a64  >/dev/null ;then
			ifconfig enx0c5b8f279a64 down
		fi
		ifconfig wlan0 up
		service wicd restart
		open_wicd_gtk
	else
		echo "none support network type"
		exit 1
	fi
	exit 0
}
case $OPTION in
	wifi)
		network_change
		;;
	4g)
		network_change
		;;
	*)
		echo "`basename $0`: usage: [--option] "
		echo -e "option:\n [wifi]  change network to wifi\n [4g]   change network to wifi\n"
		exit 1
		;;
esac

sleep 10
netstat_check

exit 0
