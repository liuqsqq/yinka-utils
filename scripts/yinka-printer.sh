#!/bin/bash -e

OPTION=$1
PRINTER=$2

function printer_add () {
	if [ $PRINTER == "brother" ]; then
		lpadmin -p brother -E -v socket://192.168.1.188 -P /usr/share/ppd/custom/brother-HL5590DN-cups.ppd
	elif [ $PRINTER == "hp" ]; then
		lpadmin -p HP_LaserJet_M403dn -E -v socket://192.168.1.189 -P /usr/share/ppd/custom/HP_LaserJet_M403dn.ppd
	else
		echo "no such printer, please check your printer name"
	fi
}

function printer_delete () {
	if [ $PRINTER == "brother" ]; then
		lpadmin -x brother
	elif [ $PRINTER == "hp" ]; then
		lpadmin -x HP_LaserJet_M403dn
	else
		echo "no such printer, please check your printer name"
	fi
}

function printer_set_default () {
	if [ $PRINTER == "brother" ]; then
		lpadmin -d brother
	elif [ $PRINTER == "hp" ]; then
		lpadmin -d HP_LaserJet_M403dn
	else
		echo "no such printer, please check your printer name"
	fi
}


case $OPTION in
	--add)
		printer_add
		;;
	--delete)
		printer_delete
		;;
	--default)
		printer_set_default
		;;
	*)
		echo "`basename $0`: usage: [--option] [printer_name]"
		echo -e "option:\n [--add]  add printer\n [--delete]  delete printer\n [--default]  set the printer to default\n"
		exit 1
		;;
esac

exit 0