/*
 *   Copyright (C) 2017  Steven Lee <geekerlw@gmail.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

#include "../common/daemon.h"

void main(int argc, char * argv[])
{
	int ret = 0;
	int delay = 1;
	
	ret = daemon_start(argc, argv);
	
	/* Never ending loop of server */
	while (ret == 1) {
		
		/* do some useful things here */
		system("yinka --use-gl=egl");

		/* wait to end loop */
		sleep(delay);
	}
	daemon_stop();
}
