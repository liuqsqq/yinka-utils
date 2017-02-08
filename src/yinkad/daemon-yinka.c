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
 
#include "../common/initd.h"
#include "../system/process.h"
#include "../system/memstat.h"
 
void main(int argc, char *argv[])
{
	int ret = 0;
	int delay = 10;
	int running = 0;

	int prostat, propid;
	float promem, procpu;

	/* config read */

	
	char *process_name = argv[1];

	ret = daemon_start(argc, argv);
	 
	/* Never ending loop of server */
	while (ret == 1) {
		 
		/* do some useful things here */
		
		/* judge whether the process is exist */
		prostat = process_status_get(process_name);
		if(prostat == 0){
			running = 1;
			printf("The %s program is already running\n", process_name);
			}
		else{
			running = 0;
			printf("There's no process named: %s, now try to restart\n", process_name);
			}
		 
		/* get process's pid */
		if(running == 1){
			propid = process_pid_get(process_name);
			printf("steven:The process %s's pid is : %d\n", process_name, propid);
			}
		else{
			printf("Can't get the process %s's pid\n", process_name);
			}
 
		/* judge whether the system and process's cpu rate and memory is too high */

		promem = process_mem_rate_get(propid);

		procpu = process_cpu_rate_get(propid);

		printf("The process %s's mem and cpu rate are : %.2f, %.2f\n", process_name, promem, procpu);
		 
 
		/* judge whether the status return of process is right */

		/* when error return, jump to restart program */

		;
		/* wait to end loop */
		sleep(delay);
	}
	daemon_stop();
}
