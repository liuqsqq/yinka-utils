#!/bin/bash 

OPTION=$1

function open_wicd_gtk () {
	wicd-gtk&
	sleep 1
	pkill -f devilspie
	/usr/local/bin/scripts/restart_devilspie.sh
}

function netstat_check () {
	if ifconfig |grep wlan0 >/dev/null;then
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
	else
		network_change
	fi
        echo "start to kill wicd"
        id=$(ps -ef | grep 'wicd-client'  | grep -v grep |  awk '{print $2}')
        if [ "$id" != "" ];then
	    pkill -f wicd-client.py
	fi
        open_wicd_gtk
}

function network_change () {
	if [ $OPTION == "4g" ]; then
		if ifconfig -a  |grep enx0c5b8f279a64  >/dev/null ;then
                        id=$(ps -ef | grep 'wicd-client'  | grep -v grep |  awk '{print $2}')
                        if [ "$id" != "" ];then
	                    pkill -f wicd-client.py
	                fi
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
	else
		echo "none support network type"
		exit 1
	fi
}
case $OPTION in
	wifi)
		netstat_check
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

exit 0
