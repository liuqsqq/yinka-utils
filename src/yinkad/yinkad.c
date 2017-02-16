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
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
 
#include "../utils/config_read.h"
#include "../utils/mem_cpu_info.h"
#include "../utils/process_info.h"


static char *conf_file_name;
static FILE *log_stream;


#define MAX_DAMEON_PROGRAMS_NUMS	2

#define DEFAULT_CONF_FILE_PATH "/etc/yinkad.conf"
#define DEFAULT_DELAY   1

typedef struct {
		char *cmdline;
		char *program_name;
		bool dameon_switch;
}program_t;

typedef struct{
	int delay;
	program_t prog_list[MAX_DAMEON_PROGRAMS_NUMS];
}daemon_config_t;

char *prog_names[MAX_DAMEON_PROGRAMS_NUMS]={
						"yinka-terminal", 
						"ads"
						};

/* typedef a global config struct */
daemon_config_t *g_daemon_config = NULL;
int running = 0;

/*
 *  Callback function for handling signals.
 * 	sig	identifier of signal
 */
void handle_signal(int sig)
{
	if (sig == SIGINT) {
        fprintf(log_stream, "Debug: stopping daemon ...\n");
		running = 0;
		/* Reset signal handling to default behavior */
		signal(SIGINT, SIG_DFL);
        fprintf(log_stream, "here\n");
	} 
	else if (sig == SIGHUP) {
		//read_conf_file();
	} 
    else if (sig == SIGCHLD) {
        fprintf(log_stream, "Debug: received SIGCHLD signal\n");
	}
	//else if (sig == SIGTEST)
	else if (sig == SIGTERM) {
	    fprintf(log_stream, "Debug: receievd SIGTERM signal\n");
        //todo
	}
}


/*
 *  Read configuration from config file
 */
int read_conf_file()
{
	FILE *conf_file = NULL;
	int ret = -1;
	int i = 0;
	char tempstr[255] = {0};
	char resultstr[255] ={0};
    
	if (conf_file_name == NULL) {
		conf_file_name = strdup(DEFAULT_CONF_FILE_PATH);
	}

	conf_file = fopen(conf_file_name, "r");

	if (conf_file == NULL) {
        fprintf(log_stream, "Can not open config file: %s, error: %s\n",
                conf_file_name, strerror(errno));
		return -1;
	}

	//fprintf(log_stream, "\nDebug: conf_file_name is %s", conf_file_name);


    ret = conf_read(conf_file_name, "General Sets", "delay", resultstr);
	if (-1 != ret) {
		g_daemon_config->delay= atoi(resultstr);
	}
	else{
		g_daemon_config->delay = DEFAULT_DELAY;
	}

	//fprintf(log_stream, "\nDebug: delay is %d", gDameonConfig->delay);
    
    for (i = 0; i < MAX_DAMEON_PROGRAMS_NUMS; i++) {  
        memset(resultstr, 0 ,255);
	    ret  = conf_read(conf_file_name, prog_names[i], "program_name", resultstr);  
        if (-1 != ret) {
	        g_daemon_config->prog_list[i].program_name = strdup(resultstr);
            memset(resultstr, 0 ,255);
    	    ret  = conf_read(conf_file_name, prog_names[i], "switch", resultstr);  
            if (-1 != ret) {
    	        if (strcmp("on", resultstr) == 0) {
        	        g_daemon_config->prog_list[i].dameon_switch = true;  
                }
                else {
                    g_daemon_config->prog_list[i].dameon_switch = false;  
                }

            }
            else {
                g_daemon_config->prog_list[i].dameon_switch = true;
            }

            memset(resultstr, 0 ,255);
    	    ret  = conf_read(conf_file_name, prog_names[i], "cmdline", resultstr);  
            if (-1 != ret) {
    	        g_daemon_config->prog_list[i].cmdline = strdup(resultstr);
            }
        }     
    }

	fclose(conf_file);

	return 0;
}


void process_monitor()
{
    int i = 0;
    int ret = -1;
    pid_t pid, status;
    float memrate = 0.0;
    float cpurate = 0.0;
    long  memvalue = 0;

    for (i = 0; i < MAX_DAMEON_PROGRAMS_NUMS; i++) {
        /* check whether dameon switch is on or off*/
        if (!g_daemon_config->prog_list[i].dameon_switch) {
            continue;
        }
        
        /* try to check program is alive, if not ,try to reboot it */
        fprintf(log_stream, "INFO: program_name = %s\n", g_daemon_config->prog_list[i].program_name);
		
    	ret = process_status_get(g_daemon_config->prog_list[i].program_name);    
    	if(ret != 0) {
			//fprintf(log_stream, "INFO: can't get program %s's status, now try to restart", g_daemon_config->prog_list[i].program_name);
    		status = system(g_daemon_config->prog_list[i].cmdline);
			if (status == -1){
				fprintf(log_stream, "ERROR: execute %s failed\n", g_daemon_config->prog_list[i].cmdline);
			}
			else{
				if(WEXITSTATUS(status) != 0){
					fprintf(log_stream, "ERROR: execute failed %d\n", WEXITSTATUS(status));
				}
			}
    	}
		else {
			fprintf(log_stream, "INFO: get program %s's status success\n", g_daemon_config->prog_list[i].program_name);
		}
        
        /* try to check program's resource */
        ret = process_pid_get(g_daemon_config->prog_list[i].program_name, &pid);
       	if (-1 != ret) {
        	process_mem_rate_get(pid, &memvalue, &memrate);
           	//cpurate = process_cpu_rate_get(pid);   
           	memrate = memrate * 100;
	       	fprintf(log_stream, "INFO: program %s used %ldM memory, memory rate is : %f%%\n\n", g_daemon_config->prog_list[i].program_name, memvalue/1000, memrate);
        }
		else {
			fprintf(log_stream, "ERROR: can't get program %s's memory info\n\n", g_daemon_config->prog_list[i].program_name);
    	}
    }
}

int yinka_dameon_init()
{
    g_daemon_config = (daemon_config_t *)malloc(sizeof(daemon_config_t));
    if (!g_daemon_config) {
        return -1;
    }
    memset(g_daemon_config, 0, sizeof(daemon_config_t));
    log_stream = stderr;
    running = 0;

    /* Daemon will handle three signals */
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);
	signal(SIGTERM, handle_signal);

    return 0;
}
void main(int argc, char *argv[])
{
    int delay = 0;
	int ret = 0;
    int i = 0;
    
    ret = yinka_dameon_init();
    if (0 != ret) {
        exit(EXIT_FAILURE);
    }
	
	/* config read */
	ret = read_conf_file();

	if (-1 != ret) {
        fprintf(log_stream, "INFO: main program's delay is: %d\n\n", g_daemon_config->delay);
        for (i = 0; i < MAX_DAMEON_PROGRAMS_NUMS; i++) {
            fprintf(log_stream, "INFO: program_name is: %s\n", g_daemon_config->prog_list[i].program_name);
            fprintf(log_stream, "INFO: program %s's dameon_switch status is: %d\n", 
					g_daemon_config->prog_list[i].program_name, g_daemon_config->prog_list[i].dameon_switch);
            fprintf(log_stream, "INFO: program %s's cmdline is: %s\n\n", 
					g_daemon_config->prog_list[i].program_name, g_daemon_config->prog_list[i].cmdline);
        }
    }

    delay = g_daemon_config->delay;
	running = 1;
    
	/* Never ending loop of server */
	while (running == 1) {
		process_monitor();
		sleep(delay);
	}

    /* Free allocated memory */
	if (conf_file_name != NULL) {
        free(conf_file_name);
    }

    for (i = 0; i < MAX_DAMEON_PROGRAMS_NUMS; i++) {
       if (NULL != g_daemon_config->prog_list[i].cmdline) {
            free(g_daemon_config->prog_list[i].cmdline); 
            g_daemon_config->prog_list[i].cmdline = NULL;
       }
       if (NULL != g_daemon_config->prog_list[i].program_name) {
            free(g_daemon_config->prog_list[i].program_name); 
            g_daemon_config->prog_list[i].program_name = NULL;
       }
    }
    if (NULL != g_daemon_config) {
		free(g_daemon_config);
        g_daemon_config = NULL;
    }
    
    fprintf(log_stream, "INFO: daemon exit\n");
}
