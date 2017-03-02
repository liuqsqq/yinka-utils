#!/bin/bash -e

OPTION=$1
VERSION=$2
FILENAME=$3

temp_dir=/tmp/updatefiles
soft_dir=/usr/local/soft
backup_dir=$soft_dir/backup

# Make sure we have the dirs we'll use
if [ ! -d "$temp_dir" ]; then
	mkdir "$temp_dir"
fi
if [ ! -d "$temp_dir/kernel" ]; then
	mkdir "$temp_dir/kernel"
fi
if [ ! -d "$temp_dir/autoprint" ]; then
	mkdir "$temp_dir/autoprint"
fi
if [ ! -d "$temp_dir/player" ]; then
	mkdir "$temp_dir/player"
fi
if [ ! -d "$temp_dir/debian" ]; then
	mkdir "$temp_dir/debian"
fi

if [ ! -d "$soft_dir" ]; then
	mkdir "$soft_dir"
fi
if [ ! -d "$soft_dir/autoprint" ]; then
	mkdir "$soft_dir/autoprint"
fi
if [ ! -d "$soft_dir/player" ]; then
	mkdir "$soft_dir/player"
fi

if [ ! -d "$backup_dir" ]; then
	mkdir "$backup_dir"
fi
if [ ! -d "$backup_dir/kernel" ]; then
	mkdir "$backup_dir/kernel"
fi
if [ ! -d "$backup_dir/autoprint" ]; then
	mkdir "$backup_dir/autoprint"
fi
if [ ! -d "$backup_dir/player" ]; then
	mkdir "$backup_dir/player"
fi


function kernel_update () {
	if [ -f "$backup_dir/kernel/zImage" ]; then
		rm -rf $backup_dir/kernel/zImage
		rm -rf $backup_dir/kernel/rk3288-firefly.dtb
	fi
	mount /dev/mmcblk2p1 /mnt
	cp -a /mnt/zImage $backup_dir/kernel/
	cp -a /mnt/rk3288-firefly.dtb $backup_dir/kernel/
	cp -a $temp_dir/kernel/* /mnt
	echo $VERSION > $soft_dir/kernel_version.ver
	umount /mnt
}

function autoprint_update () {
	if [ -d "$backup_dir/autoprint" ]; then
		rm -rf $backup_dir/autoprint
	fi
	cp -a $soft_dir/autoprint $backup_dir/
	cp -a $temp_dir/autoprint $soft_dir/
	echo $VERSION > $soft_dir/autoprint_version.ver
	ln -sf $soft_dir/autoprint/autoprint /usr/bin/autoprint
}

function player_update () {
    if [ -d "$backup_dir/player" ]; then
        rm -rf $backup_dir/player
    fi
	cp -a $soft_dir/player $backup_dir/
	cp -a $temp_dir/player $soft_dir/
	echo $VERSION > $soft_dir/player_version.ver
	ln -sf $soft_dir/player/PRAdsPlayer /usr/bin/PRAdsPlayer
}

function debian_update () {
	sh $temp_dir/debian/$FILENAME
	echo $VERSION > $soft_dir/"$FILENAME"_version.ver
}

case $OPTION in
	--kernel)
		kernel_update
		;;
	--print)
		autoprint_update
		;;
	--player)
		player_update
		;;
	--debian)
		debian_update
		;;
	*)
		echo "`basename $0`: usage: [--kernel kernel_version] | [--print print_version] | [--player player_version] | [--debian package_version install_sh]"
		exit 1
		;;
esac

exit 0
