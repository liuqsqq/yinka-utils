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
#include <sys/types.h> 
#include <sys/wait.h> 
#include <fcntl.h> 
#include <limits.h> 

#include "process.h"

static void err_quit(char *msg) 
{ 
	perror(msg); 
	exit(EXIT_FAILURE); 
} 

/* process pid get */
int process_pid_get(char *process_name)
{	
	FILE *fp;
	char pidof[POPENSIZE];
	char pidbuf[BUFFSIZE];
	pid_t pid;

	sprintf(pidof, "pidof -s %s", process_name);
	fp = popen(pidof, "r");
	
	if(fp == NULL){
		err_quit("popen");
		}
	else{
		if(fgets(pidbuf, sizeof(pid), fp) != NULL)
			{
				printf("The process: %s's pid is: %s\n", process_name, pidbuf);
				pid = atoi(pidbuf);
				return pid;
			}
		else{
				printf("can't get process id\n ");
				return 0;
		}
	}

	pclose(fp); 
	exit(EXIT_SUCCESS); 
}

int process_status_get(char *process_name) 
{ 
	FILE* fp;
	int count = 0;
	char ps[POPENSIZE];
	char psbuf[BUFFSIZE];  
	
	sprintf(ps, "ps -e | grep -c %s", process_name); 
	fp = popen(ps,"r");

	if(fp == NULL){
		err_quit("popen");
		}
	else{
		if(fgets(psbuf, sizeof(psbuf), fp) != NULL)
			{
			count = atoi(psbuf);
			if(count == 0)
				{
					printf("process %s not found\n", process_name);
					return 1;
				}
			else{
					printf("program %s has %d process\n", process_name, count);
					return 0;
				}
			}
		}
	
	pclose(fp); 
	exit(EXIT_SUCCESS); 
} 
