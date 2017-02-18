# yinka-utils
===
## functions 
   This source can build some bins used on yinka board, such as a daemon program named yinkad, we will add our updater program in it soon. you can put the yinkad.conf to /etc/yinkad.conf like this:  

	# set general delay value
	[General Sets]
	delay = 10

	# set process name and monitor status by on/off
	[yinka-terminal]
	cmdline = cmdline &
	program_name = process_name
	switch = on

## how to build
	./autogen.sh    
	./configure --prefix=/usr  
	make && sudo make install  

## build a deb package
	DEB_BUILD_OPTIONS=nocheck debuild -i -nc -us -uc -b -d -aarmhf  
