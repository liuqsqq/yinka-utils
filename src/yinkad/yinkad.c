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
#include <pthread.h>
#include <sys/socket.h> 
#include <sys/un.h>
#include<netinet/in.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include<time.h>

 
#include "../utils/config_read.h"
#include "../utils/mem_cpu_info.h"
#include "../utils/process_info.h"


static char *conf_file_name;
static FILE *log_stream;
static int g_yinka_daemon_sock;

#define MAX_DAMEON_PROGRAMS_NUMS		2
#define MAX_KEEPALIVE_FAILED_TIMES 		10
#define DEFAULT_DELAY   				1
#define YINKA_DAEMON_PORT  				12332 

#define DEFAULT_CONF_FILE_PATH "/etc/yinkad.conf"

typedef struct {
    unsigned short type;
    unsigned short len;
    char data[0];
}yinka_daemon_tlv_t;

typedef struct {
    int version;
    char *prog_name;
    float cpurate;
    float memrate;
    long uptime;
    int reboot_times;
    int keepalive_failed_times; 
}program_state_t;

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
program_state_t  g_prog_state_list[MAX_DAMEON_PROGRAMS_NUMS] = {0};

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
			g_prog_state_list[i].reboot_times++;
            g_prog_state_list[i].uptime = time(NULL);
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
			g_prog_state_list[i].memrate = memrate;
	       	fprintf(log_stream, "INFO: program %s used %ldM memory, memory rate is : %f%%\n\n", g_daemon_config->prog_list[i].program_name, memvalue/1000, memrate);
        }
		else {
			fprintf(log_stream, "ERROR: can't get program %s's memory info\n\n", g_daemon_config->prog_list[i].program_name);
    	}
    }
}

void process_keepalive()
{
    int i = 0;
    for (i = 0; i < MAX_DAMEON_PROGRAMS_NUMS; i++ ) {
        /* check whether dameon switch is on or off*/
        if (!g_daemon_config->prog_list[i].dameon_switch) {
            continue;
        }
        if (MAX_KEEPALIVE_FAILED_TIMES < g_prog_state_list[i].keepalive_failed_times) {
            fprintf(log_stream, "INFO: keepalive timeout, try to restart %s\n", g_daemon_config->prog_list[i].program_name);
            g_prog_state_list[i].keepalive_failed_times = 0;
            system(g_daemon_config->prog_list[i].cmdline);
        }
        else {
            g_prog_state_list[i].keepalive_failed_times++;
        }
    }
}

int process_data_receive(char *ptr)
{
    char buff[512];
    char *yinka_daemon_tmp= NULL;
    char *control_cmd_tmp = NULL;
    yinka_daemon_tlv_t *control_cmd = NULL;
    yinka_daemon_tlv_t *yinka_daemon = NULL;
    
    fd_set set;
    struct timeval timeout;    
    struct sockaddr_in client_addr;
    int recv_bytes;
    int max_fd;
    int nfound;
    int type;
    int value_len;
    int data_len = 0;
    int control_cmd_data_len = 0;
    int client_addr_len;
    client_addr_len = sizeof(client_addr);  
    
    while (1)
    {
        data_len = 0;
        control_cmd_data_len = 0;
        FD_ZERO(&set);
        FD_SET(g_yinka_daemon_sock, &set);
        max_fd  = g_yinka_daemon_sock;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        nfound = select(max_fd + 1, &set, (fd_set *)0, (fd_set *)0, &timeout);
        if(nfound  < 0)          
        {
            fprintf(log_stream, "ERROR: select error!\n");
            continue;
        }
        
        #if 0
        else if (nfound == 0)
        {
            fprintf(log_stream, "\nDebug: select timeout!");
            continue;
        }
        #endif 
        
        if (FD_ISSET(g_yinka_daemon_sock, &set))
        {        
            recv_bytes = recvfrom(g_yinka_daemon_sock, buff, 511, 0, (struct sockaddr*)&client_addr, &client_addr_len);
            if (recv_bytes > 0)
            {
                buff[recv_bytes] = 0;
                fprintf(log_stream,"INFO: Receive %d bytes\n", recv_bytes); 
                yinka_daemon_tmp = buff;    
                while(data_len < recv_bytes)
                {
                    control_cmd = (yinka_daemon_tlv_t *)yinka_daemon_tmp;
                    type = ntohs(control_cmd->type);
                    data_len += sizeof(unsigned short);
                    data_len += sizeof(unsigned short);
                    value_len = ntohs(control_cmd->len);
                    
                    #if 0
                    fprintf(log_stream, "\nDebug:type:%d,len=%d", type, value_len); 
                    fprintf(log_stream, "\nDebug:value:");
                    for (int i = 0; i < value_len; i++)
                    {
                        fprintf(log_stream, "%2x ", pYinkaDameon->data[i]); 
                    }
                    #endif
                    
                    /* deal control cmd*/
                    if (type == 0x00)
                    {
                        control_cmd_tmp = (yinka_daemon_tmp + data_len);
                        while ( control_cmd_data_len < value_len )
                        {
                            control_cmd = (yinka_daemon_tlv_t *)control_cmd_tmp;
                            type = ntohs(control_cmd->type);
                            control_cmd_data_len += sizeof(unsigned short);
                            control_cmd_data_len += sizeof(unsigned short);
                            
                            #if 0
                            fprintf(log_stream, "\n    Debug:subtype:%d,len=%d", type, ntohs(pControlCmd->len));
                            fprintf(log_stream, "\n    Debug:value:");
                            for (int j = 0; j < ntohs(pControlCmd->len); j++)
                            {
                                fprintf(log_stream, "%2x ", pControlCmd->data[j]); 
                            }
                            #endif
                            
                            if ( (type == 0x01) || (type == 0x02))
                            {
                                if (control_cmd->data[0] == 0x01)
                                    g_daemon_config->prog_list[type-1].dameon_switch = true;
                                else if (control_cmd->data[0] == 0x00)
                                    g_daemon_config->prog_list[type-1].dameon_switch = false;
                                else if (control_cmd->data[0] == 0x02)
                                {
                                   //get program's status
                                   fprintf(log_stream, "Prog_name:%s\n", g_prog_state_list[type-1].prog_name);   
                                   fprintf(log_stream, "Version:%d\n", g_prog_state_list[type-1].version);   
                                   fprintf(log_stream, "Uptime:%ld\n", g_prog_state_list[type-1].uptime);
                                   fprintf(log_stream, "Memrate:%f\n", g_prog_state_list[type-1].memrate);
                                   fprintf(log_stream, "Cpurate:%f\n", g_prog_state_list[type-1].cpurate);
                                   fprintf(log_stream, "Reboot_times:%d\n\n", g_prog_state_list[type-1].reboot_times);
                                }
                                if ((control_cmd->data[0] == 0x01) || (control_cmd->data[0] == 0x02))
                                    fprintf(log_stream, "Control cmd:%s program %s's dameon\n",g_daemon_config->prog_list[type-1].dameon_switch?"open":"close",\
                                    g_daemon_config->prog_list[type-1].program_name);
                            }
                            else if (type == 0xffff)
                            {
                                if (control_cmd->data[0] == 0x01)
                                {
                                    for (int k = 0; k < MAX_DAMEON_PROGRAMS_NUMS; k++)
                                    {
                                        g_daemon_config->prog_list[k].dameon_switch = true;
                                    }   
                                }                        
                                else if (control_cmd->data[0] == 0x00)
                                {
                                    for (int k = 0; k < MAX_DAMEON_PROGRAMS_NUMS; k++)
                                    {
                                       g_daemon_config->prog_list[k].dameon_switch = false;
                                    }   
                                }   
                                else if (control_cmd->data[0] == 0x02)
                                {
                                    //get all program's status
                                } 
                                fprintf(log_stream, "Control cmd:close all programs' dameon\n");
                            }

                            control_cmd_data_len += ntohs(control_cmd->len);
                            control_cmd_tmp += control_cmd_data_len;
                        }
                        
                    }
                    /* deal keepalive cmd*/
                    else if (type == 0x01)
                    {
                        //fprintf(log_stream, "\nDebug:value:%d", ntohl(*((int*)pKeepAlive->data)));
                        g_prog_state_list[control_cmd->data[0]].keepalive_failed_times = 0;
                    
}
                    data_len += value_len;
                    yinka_daemon_tmp += data_len;
                }
                //fprintf(log_stream, "\n%s %u says: %s\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), buff);
            }
            else
            {
                perror("recv");
                break;
            }       
        }
    }
    close(g_yinka_daemon_sock);
    return 0;
}

int yinka_daemon_server_init()
{
	int on = 1;
    pthread_t  yinka_daemon_thread;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(YINKA_DAEMON_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if ( (g_yinka_daemon_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
  
    if((setsockopt(g_yinka_daemon_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))<0) {  
        perror("setsockopt failed");  
        exit(EXIT_FAILURE);  
    }  
    
    if (bind(g_yinka_daemon_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    } 
    
    pthread_create(&yinka_daemon_thread, NULL, (void *)(&process_data_receive), NULL); 
    return 0;
}

int yinka_dameon_init()
{
    g_daemon_config = (daemon_config_t *)malloc(sizeof(daemon_config_t));
    if (!g_daemon_config) {
        return -1;
    }
	
    memset(g_daemon_config, 0, sizeof(daemon_config_t));
	memset(g_prog_state_list, 0, sizeof(program_state_t)); 
    log_stream = stderr;
    running = 0;
	
	for (int i = 0; i < MAX_DAMEON_PROGRAMS_NUMS; i++)
    {
        g_prog_state_list[i].prog_name = prog_names[i];
    }

	yinka_daemon_server_init();
		
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
		process_keepalive();
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
