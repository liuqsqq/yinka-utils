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
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>
 

#include "../utils/config_read.h"
#include "../utils/mem_cpu_info.h"
#include "../utils/process.h"




static char *conf_file_name;
static FILE *log_stream;


#define MAX_DAMEON_PROGRAMS_NUMS    (2)

#define DEFAULT_CONF_FILE_PATH "/etc/yinkad.conf"
#define DEFAULT_DELAY   (1)

typedef struct {
		char *cmdline;
		char *program_name;
		bool dameon_switch;
}PROGRAM_T;

typedef struct{
	int delay;
	PROGRAM_T prog_list[MAX_DAMEON_PROGRAMS_NUMS] ;
}DAEMON_CONFIG_T;

char *prog_names[MAX_DAMEON_PROGRAMS_NUMS]={"yinka-terminal", "ads"};

/* typedef a global config struct */
DAEMON_CONFIG_T *gDameonConfig = NULL;
int running = 0;

#define SIGTEST   (__SIGRTMIN+10)
//void handle_signal(int sig);

/*
 *  Callback function for handling signals.
 * 	sig	identifier of signal
 */
void handle_signal(int sig)
{
	if (sig == SIGINT) 
	{
        fprintf(log_stream, "\nDebug: stopping daemon ...\n");
		running = 0;
		/* Reset signal handling to default behavior */
		signal(SIGINT, SIG_DFL);
        fprintf(log_stream, "\nhere");
	} 
	else if (sig == SIGHUP) 
	{
		//read_conf_file();
	} 
    else if (sig == SIGCHLD) 
    {
        fprintf(log_stream, "\nDebug: received SIGCHLD signal");
	}
	else if (sig == SIGTEST)
	{
	    fprintf(log_stream, "\nDebug: receievd SIGTEST signal");
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
	char tempstr[255] = {0};
	char resultstr[255] ={0};
	int i = 0;
    
	if (conf_file_name == NULL)
	{
		conf_file_name = strdup(DEFAULT_CONF_FILE_PATH);
	}

	conf_file = fopen(conf_file_name, "r");

	if (conf_file == NULL) 
	{
        fprintf(log_stream, "Can not open config file: %s, error: %s\n",
                conf_file_name, strerror(errno));
		return -1;
	}

	//fprintf(log_stream, "\nDebug: conf_file_name is %s", conf_file_name);


    ret = conf_read(conf_file_name, "General Sets", "delay", resultstr);
	if (-1 != ret)
	{
		gDameonConfig->delay= atoi(resultstr);
	}
	else
	{
		gDameonConfig->delay = DEFAULT_DELAY;
	}

	//fprintf(log_stream, "\nDebug: delay is %d", gDameonConfig->delay);
    
    for (i = 0; i < MAX_DAMEON_PROGRAMS_NUMS; i++ )
    {  
        memset(resultstr, 0 ,255);
	    ret  = conf_read(conf_file_name, prog_names[i], "program_name", resultstr);  
        if (-1 != ret)	
	    {
	        gDameonConfig->prog_list[i].program_name = strdup(resultstr);
            memset(resultstr, 0 ,255);
    	    ret  = conf_read(conf_file_name, prog_names[i], "switch", resultstr);  
            if (-1 != ret)	
    	    {
    	        if (strcmp("on", resultstr) == 0)
        	    {
        	        gDameonConfig->prog_list[i].dameon_switch = true;  
                }
                else
                {
                    gDameonConfig->prog_list[i].dameon_switch = false;  
                }

            }
            else
            {
                gDameonConfig->prog_list[i].dameon_switch = true;
            }

            memset(resultstr, 0 ,255);
    	    ret  = conf_read(conf_file_name, prog_names[i], "cmdline", resultstr);  
            if (-1 != ret)	
    	    {
    	        gDameonConfig->prog_list[i].cmdline = strdup(resultstr);
            }
        }     
    }

	fclose(conf_file);

	return 0;
}


void process_monitor_program()
{
    int i = 0;
    int ret = -1;
    pid_t pid;
    float memrate = 0.0;
    float cpurate = 0.0;
    long  memvalue = 0;

    for (i = 0; i < MAX_DAMEON_PROGRAMS_NUMS; i++ )
    {
        /* check whether dameon switch is on or off*/
        if (!gDameonConfig->prog_list[i].dameon_switch)
        {
            continue;
        }
        
        /* try to check program is alive, if not ,try to reboot it */
        fprintf(log_stream, "\nDebug: program_name = %s", gDameonConfig->prog_list[i].program_name);
    	ret = process_status_get(gDameonConfig->prog_list[i].program_name);
        fprintf(log_stream, "\nDebug: process_status_get ret = %d", ret);
    	if(ret != 0)
        {   
            system("export DISPLAY=:0.0");
    		system(gDameonConfig->prog_list[i].cmdline);          
            fprintf(log_stream, "\nDebug: execute %s", gDameonConfig->prog_list[i].cmdline);
    	}; 
        
        
        /* try to check program's resource */
        ret = process_pid_get(gDameonConfig->prog_list[i].program_name, &pid);
        fprintf(log_stream, "\nDebug: process_pid_get ret = %d, pid = %d", ret, pid);
       	if (-1 != ret)
        {
           process_mem_rate_get(pid, &memvalue, &memrate);
           //cpurate = process_cpu_rate_get(pid);   
           memrate = memrate * 100;
	       fprintf(log_stream, "\nDebug: memrate of program %s is %f%%, value is %ld", gDameonConfig->prog_list[i].program_name, memrate, memvalue/1000);
        }

    }
}

int yinka_dameon_init()
{
    gDameonConfig = (DAEMON_CONFIG_T *)malloc(sizeof(DAEMON_CONFIG_T));
    if (!gDameonConfig)
    {
        return -1;
    }
    memset(gDameonConfig, 0, sizeof(DAEMON_CONFIG_T));
    log_stream = stderr;
    running = 0;

    /* Daemon will handle three signals */
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);
	signal(SIGTEST, handle_signal);

    return 0;
}
void main(int argc, char *argv[])
{
    int delay = 0;
	int ret = 0;
    int i = 0;
    
    ret = yinka_dameon_init();
    if (0 != ret)
    {
        exit(EXIT_FAILURE);
    }
	/* config read */
	ret = read_conf_file();
    if (-1 != ret)
    {
        fprintf(log_stream, "\nDebug: delay is %d", gDameonConfig->delay);
        for (i = 0; i < MAX_DAMEON_PROGRAMS_NUMS; i++ )
        {
            fprintf(log_stream, "\nDebug: program_name is %s", gDameonConfig->prog_list[i].program_name);
            fprintf(log_stream, "\nDebug: dameon_switch is %d", gDameonConfig->prog_list[i].dameon_switch);
            fprintf(log_stream, "\nDebug: cmdline is %s", gDameonConfig->prog_list[i].cmdline);
        }
        fprintf(log_stream, "\nhere");
    }


    delay = gDameonConfig->delay;
	running = 1;
    
	/* Never ending loop of server */
	while (running == 1) 
    {
		process_monitor_program();
		sleep(delay);
	}

    /* Free allocated memory */
	if (conf_file_name != NULL) 
    {
        free(conf_file_name);
    }

    
    
    for (i = 0; i < MAX_DAMEON_PROGRAMS_NUMS; i++ )
    {
       if (NULL != gDameonConfig->prog_list[i].cmdline)
       {
            free(gDameonConfig->prog_list[i].cmdline); 
            gDameonConfig->prog_list[i].cmdline = NULL;
       }
       if (NULL != gDameonConfig->prog_list[i].program_name)
       {
            free(gDameonConfig->prog_list[i].program_name); 
            gDameonConfig->prog_list[i].program_name = NULL;
       }
       if (NULL != gDameonConfig)
       {
            free(gDameonConfig); 
            gDameonConfig = NULL;
       }
       
    }
    
    fprintf(log_stream, "\nhere2");
}
