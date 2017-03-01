#!/bin/bash -e

NAME=$1
OPTION=$2

id=($(xinput list | grep $NAME | awk -F= '{print $2}' | awk '{print $1}'))

for DEVICE in ${id[*]}
do
	if [ $DEVICE != " " ]; then
		if [ $OPTION = "on" ]; then
			xinput --set-prop $DEVICE "Device Enabled" 1
		elif [ $OPTION = "off" ]; then
			xinput --set-prop $DEVICE "Device Enabled" 0
		else
			echo "none support option"
			exit 1
		fi
	fi
done

exit 0
