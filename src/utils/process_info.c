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
#include <syslog.h>
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/wait.h> 
#include <fcntl.h> 
#include <limits.h> 

#include "process_info.h"


static void err_quit(char *msg) 
{ 
	perror(msg); 
	exit(EXIT_FAILURE); 
} 

/* process pid get */
int process_pid_get(char *process_name, pid_t *pid)
{	
	FILE *fp;
	char pidof[POPENSIZE];
	char pidbuf[BUFFSIZE];
	
	sprintf(pidof, "pidof %s", process_name);
	fp = popen(pidof, "r");
	
	if(fp == NULL) {
		err_quit("popen");
	}
	else {
		if(fgets(pidbuf, sizeof(pidbuf), fp) != NULL) {
			//fprintf(stderr, "The process: %s's pid is: %s\n", process_name, pidbuf);
			*pid = atoi(pidbuf);
			
			pclose(fp);
			return 0;
		}
		else {
			//fprintf(stderr, "can't get process id\n ");
			pclose(fp);
			return -1;
		}
	}
}

int process_status_get(char *process_name) 
{ 
	FILE* fp;
	int count = 0;
	char ps[POPENSIZE];
	char psbuf[BUFFSIZE];  
	
	sprintf(ps, "ps -ef | grep  %s | grep -c -v grep", process_name); 
	fp = popen(ps,"r");

	if(fp == NULL) {
		err_quit("popen");
	}
	else {
		if(fgets(psbuf, sizeof(psbuf), fp) != NULL) {
			count = atoi(psbuf);
			if(count == 0) {
				//fprintf(stderr, "process %s not found\n", process_name);

				pclose(fp);
				return -1;
			}
			else {
				//fprintf(stderr, "program %s has %d process\n", process_name, count);
				pclose(fp);
				return 0;
			}
		}
	}
} 
