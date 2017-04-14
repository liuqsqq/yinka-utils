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
#include <time.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/socket.h> 
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>

#include "yinkad.h"
#include "../utils/config_read.h"
#include "../utils/mem_cpu_info.h"
#include "../utils/process_info.h"


static char *conf_file_name;
static FILE *log_stream;
static int g_yinka_daemon_sock;
static int running = 0;

/* typedef global config struct */
daemon_config_t *g_daemon_config = NULL;
program_state_t  g_prog_state_list[MAX_DAMEON_PROGRAMS_NUMS] = {0};

/*
 *  Read configuration from config file
 */
static int read_conf_file()
{
	FILE *conf_file = NULL;
	int ret = -1;
	int i = 0;
	char tempstr[MAX_BUFFER_LEN] = {0};
	char resultstr[MAX_BUFFER_LEN] ={0};
    
	if (conf_file_name == NULL) {
		conf_file_name = strdup(DEFAULT_CONF_FILE_PATH);
	}

	conf_file = fopen(conf_file_name, "r");

	if (conf_file == NULL) {
        fprintf(log_stream, "ERROR: Can not open config file: %s, error: %s\n",
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
        memset(resultstr, 0 ,MAX_BUFFER_LEN);
	    ret  = conf_read(conf_file_name, prog_names[i], "program_name", resultstr);  
        if (-1 != ret) {
	        g_daemon_config->prog_list[i].program_name = strdup(resultstr);
            memset(resultstr, 0 ,MAX_BUFFER_LEN);
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

            memset(resultstr, 0 ,MAX_BUFFER_LEN);
    	    ret  = conf_read(conf_file_name, prog_names[i], "cmdline", resultstr);  
            if (-1 != ret) {
    	        g_daemon_config->prog_list[i].cmdline = strdup(resultstr);
            }
        }     
    }

	fclose(conf_file);

	return 0;
}

/*
 *  Callback function for handling signals.
 * 	sig	identifier of signal
 */
static void handle_signal(int sig)
{
	if (sig == SIGINT) {
        fprintf(log_stream, "INFO: stopping daemon ...\n");
		running = 0;
		/* Reset signal handling to default behavior */
		signal(SIGINT, SIG_DFL);
        fprintf(log_stream, "INFO: reset signal handing to default behavior\n");
	} 
	else if (sig == SIGHUP) {
		if(!read_conf_file()){
			fprintf(log_stream, "INFO: reload daemon config\n");
		}
	} 
    else if (sig == SIGCHLD) {
        fprintf(log_stream, "INFO: received SIGCHLD signal\n");
	}
}

static int getFatherPid(int pid)
{
    char dir[1024]={0};
    char path[1024] = {0};
    char buf[1024] = {0};
    int rpid = 0;
    int fpid=0;
    char fpth[1024]={0};
    struct stat st;
    ssize_t ret =0;

    sprintf(dir,"/proc/%d/",pid);

    sprintf(path,"%sstat",dir);

    if(stat(path,&st)!=0)
    {
        return -2; 
    }

    memset(buf,0,strlen(buf));

    FILE * fp = fopen(path,"r");

    ret += fread(buf + ret,1,300-ret,fp);

    fclose(fp);

    sscanf(buf,"%*d %*c%s %*c %d %*s",fpth,&fpid);

    fpth[strlen(fpth)-1]='\0';

    if(strcmp(fpth,"bash")!=0 && strcmp(fpth,"sudo")!=0 ) //bash ÖÕ¶Ë  sudo ÖÕ¶Ë
    {
        if(fpid==1)
        {
            return pid;
        }
        else if(fpid==2)
        {
            return -1; //ÄÚºËÏß³Ì
        }

        rpid = getFatherPid(fpid);

        if(rpid == 0) 
        {
           rpid  = pid;
        }
    }

    return rpid;
}

static void process_kill(char *program_name)
{
	pid_t kill_status;
	char cmd_str[256];
    pid_t pid = -1;
    pid_t ppid = -1;
    process_pid_get(program_name, &pid);
    ppid = getFatherPid(pid);     
    sprintf(cmd_str, "kill %d", ppid);
    kill_status = system(cmd_str);
    
	if (kill_status == -1){
		fprintf(log_stream, "ERROR: execute %s failed\n", cmd_str);
	}
	else{
		if(WEXITSTATUS(kill_status) != 0){
			fprintf(log_stream, "ERROR: execute failed %d\n", WEXITSTATUS(kill_status));
		}
	}
}


static void process_restart(char *program_name, char *cmdline)
{
	pid_t restart_status;
    
    process_kill(program_name);
    restart_status = system(cmdline);
    
	if (restart_status == -1){
		fprintf(log_stream, "ERROR: execute %s failed\n", cmdline);
	}
	else{
		if(WEXITSTATUS(restart_status) != 0){
			fprintf(log_stream, "ERROR: execute failed %d\n", WEXITSTATUS(restart_status));
		}
	}
}


static void process_keepalive()
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
            process_restart(g_daemon_config->prog_list[i].program_name, g_daemon_config->prog_list[i].cmdline);
        }
        else {
            g_prog_state_list[i].keepalive_failed_times++;
        }
    }
}

static void process_monitor()
{
    int i = 0;
    int ret = -1;
    pid_t pid;
    float memrate = 0.0;
    float cpurate = 0.0;
    long  memvalue = 0;

    for (i = 0; i < MAX_DAMEON_PROGRAMS_NUMS; i++) {
        /* check whether dameon switch is on or off*/
        if (!g_daemon_config->prog_list[i].dameon_switch) {
            continue;
        }

        if (g_daemon_config->prog_list[i].safe_restart){
			fprintf(log_stream, "INFO: program %s received update restart cmd, now try to restart\n", g_daemon_config->prog_list[i].program_name);
    		process_restart(g_daemon_config->prog_list[i].program_name, g_daemon_config->prog_list[i].cmdline);
			g_prog_state_list[i].reboot_times++;
            g_prog_state_list[i].uptime = time(NULL); 
            g_daemon_config->prog_list[i].safe_restart = false;
            continue;
        }
        /* try to check program is alive, if not ,try to reboot it */
        //fprintf(log_stream, "INFO: program_name = %s\n", g_daemon_config->prog_list[i].program_name);
		
    	ret = process_status_get(g_daemon_config->prog_list[i].program_name);    
    	if(ret == 0) {
			//fprintf(log_stream, "INFO: get program %s's status success\n", g_daemon_config->prog_list[i].program_name);

			/* try to check program's resource */
			ret = process_pid_get(g_daemon_config->prog_list[i].program_name, &pid);
       		if (ret == 0) {
        		process_mem_rate_get(pid, &memvalue, &memrate);
           		//cpurate = process_cpu_rate_get(pid);   
				g_prog_state_list[i].memrate = memrate * 100 *1000;
	       		//fprintf(log_stream, "INFO: program %s used %ldM memory, memory rate is : %f%%\n\n", g_daemon_config->prog_list[i].program_name, memvalue/1000, memrate);
        	}
			else {
				fprintf(log_stream, "ERROR: can't get program %s's memory info\n\n", g_daemon_config->prog_list[i].program_name);
    		}
    	}
		else {
			fprintf(log_stream, "ERROR: can't get program %s's status, now try to restart\n", g_daemon_config->prog_list[i].program_name);
    		process_restart(g_daemon_config->prog_list[i].program_name, g_daemon_config->prog_list[i].cmdline);
			g_prog_state_list[i].reboot_times++;
            g_prog_state_list[i].uptime = time(NULL);
		}
    }
}
int daemon_data_send(struct sockaddr_in *client_addr, program_state_t* proram_statistic, int prog_nums)
{
    int x,y;
    char buffer[MAX_BUFFER_LEN];
    int buffer_len = 0;
    char * pstr = NULL;
    program_state_t * pvalue = NULL;
    yinka_daemon_tlv_t * ptemp = (yinka_daemon_tlv_t *)buffer;
    int ret = 0;

    if ((client_addr == NULL) || (proram_statistic == NULL))
        return -1;
 
    ptemp->type = htons(TYPE_RES_STATISTIC);

    *(unsigned short *)ptemp->data = htons(prog_nums);
    
    pstr =  ptemp->data + sizeof(unsigned short);   
    pvalue = (program_state_t *)pstr;

    buffer_len += sizeof(unsigned short);

    for (int j = 0; j < prog_nums; j++){
        memcpy(pvalue->prog_name, proram_statistic[j].prog_name, MAX_STR_LEN);
        pvalue->version = htonl(proram_statistic[j].version);
        pvalue->uptime = htonl(proram_statistic[j].uptime);
        pvalue->cpurate = htonl(proram_statistic[j].cpurate);
        pvalue->memrate= htonl(proram_statistic[j].memrate);
        pvalue->reboot_times = htonl(proram_statistic[j].reboot_times);

        buffer_len += sizeof(program_state_t);  
        pvalue++;
    }

    ptemp->len = htons(buffer_len);
    ret = sendto(g_yinka_daemon_sock, buffer, buffer_len, 0,
            (struct sockaddr*)client_addr, sizeof(struct sockaddr_in));
    if (ret < 0){
        return -1;
    }
    return 0;
}

static int process_data_receive(char *ptr)
{
    char buff[MAX_BUFFER_LEN];
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
    int send_result = 0;    

    unsigned short program_id;
    int  time_stramp;
    unsigned short state;

    int k = 0;

    while (1) {
        data_len = 0;
        control_cmd_data_len = 0;
        FD_ZERO(&set);
        FD_SET(g_yinka_daemon_sock, &set);
        max_fd  = g_yinka_daemon_sock;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        nfound = select(max_fd + 1, &set, (fd_set *)0, (fd_set *)0, &timeout);
        if(nfound  < 0) {
            fprintf(log_stream, "ERROR: select error!\n");
            continue;
        }
        if (FD_ISSET(g_yinka_daemon_sock, &set)) {        
            recv_bytes = recvfrom(g_yinka_daemon_sock, buff, MAX_BUFFER_LEN, 0, (struct sockaddr*)&client_addr, &client_addr_len);
            if (recv_bytes > 0) {
                buff[recv_bytes] = 0;
                #if 0
                fprintf(log_stream,"INFO: Receive %d bytes\n", recv_bytes); 
                for (int j = 0; j < recv_bytes; j++)
                {
                    fprintf(log_stream, "%2x ", buff[j]); 
                } 
                #endif
                yinka_daemon_tmp = buff;    
                while(data_len < recv_bytes) {
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
                        fprintf(log_stream, "%2x ", control_cmd->data[i]); 
                    }
                    #endif                    
                    /* deal control cmd*/
                    if (type == TYPE_CONTROL_CMD) {
                        control_cmd_tmp = (yinka_daemon_tmp + data_len);
                        while ( control_cmd_data_len < value_len ) {
                            control_cmd = (yinka_daemon_tlv_t *)control_cmd_tmp;
                            type = ntohs(control_cmd->type);
                            control_cmd_data_len += sizeof(unsigned short);
                            control_cmd_data_len += sizeof(unsigned short);                  
                            if ( (type == YINKA_PRINT) || (type == YINKA_PLAYER)) {
                                if (control_cmd->data[0] == DAEMON_ON)
                                    g_daemon_config->prog_list[type-1].dameon_switch = true;
                                else if (control_cmd->data[0] == DAEMON_OFF)
                                    g_daemon_config->prog_list[type-1].dameon_switch = false;
                                else if (control_cmd->data[0] == DAEMON_GETINFO) {
                                   send_result = daemon_data_send(&client_addr, &g_prog_state_list[type-1], 1);
                                   if (send_result != 0)
                                   {
                                       fprintf(log_stream, "ERROR: send program data faield\n");  
                                   }
                                   else
                                   {
                                       fprintf(log_stream, "INFO: send program data successed\n");
                                   }
                                }
                                else if (control_cmd->data[0] == DAEMON_SAFE_RESTART) {
                                   fprintf(log_stream, "INFO: start to restart %s safety\n", g_daemon_config->prog_list[type-1].program_name);
                                   if (g_prog_state_list[type-1].state == IS_BUSY)
                                       g_daemon_config->prog_list[type-1].safe_restart = true;
                                   else{
                                        process_restart(g_daemon_config->prog_list[type-1].program_name, g_daemon_config->prog_list[type-1].cmdline);
                            			g_prog_state_list[type-1].reboot_times++;
                                        g_prog_state_list[type-1].uptime = time(NULL); 
                                   }

                                }
                                else if (control_cmd->data[0] == DAEMON_FORCE_RESTART) {
                                        fprintf(log_stream, "INFO: start to restart %s forced\n", g_daemon_config->prog_list[type-1].program_name);
                                        g_daemon_config->prog_list[type-1].dameon_switch = true;
                                        process_restart(g_daemon_config->prog_list[type-1].program_name, g_daemon_config->prog_list[type-1].cmdline);
                            			g_prog_state_list[type-1].reboot_times++;
                                        g_prog_state_list[type-1].uptime = time(NULL); 
                                }
                                else if(control_cmd->data[0] == DAEMON_CLOSE) {
                                        g_daemon_config->prog_list[type-1].dameon_switch = false;
                                        fprintf(log_stream, "INFO: start to close %s forced\n", g_daemon_config->prog_list[type-1].program_name);
                                        process_kill(g_daemon_config->prog_list[type-1].program_name); 
                                }
                                if ((control_cmd->data[0] == DAEMON_OFF) || (control_cmd->data[0] == DAEMON_ON))
                                    fprintf(log_stream, "INFO: Control cmd:%s program %s's dameon\n",
                                    g_daemon_config->prog_list[type-1].dameon_switch?"open":"close",
                                    g_daemon_config->prog_list[type-1].program_name);
                            }
                            else if (type == YINKA_ALL) {
                                if (control_cmd->data[0] == DAEMON_ON) {
                                    for (k = 0; k < MAX_DAMEON_PROGRAMS_NUMS; k++) {
                                        g_daemon_config->prog_list[k].dameon_switch = true;
                                    }   
                                }                        
                                else if (control_cmd->data[0] == DAEMON_OFF) {
                                    for (k = 0; k < MAX_DAMEON_PROGRAMS_NUMS; k++) {
                                       g_daemon_config->prog_list[k].dameon_switch = false;
                                    }   
                                }   
                                else if (control_cmd->data[0] == DAEMON_GETINFO) {
                                     //get all program's status
                                    send_result = daemon_data_send(&client_addr, g_prog_state_list, MAX_DAMEON_PROGRAMS_NUMS);
                                    if (send_result != 0) {
                                        fprintf(log_stream, "ERROR: send program data faield\n");  
                                    }
                                    else {
                                        fprintf(log_stream, "INFO: send program data successed \n");
                                    }
                                } 
                                else if (control_cmd->data[0] == DAEMON_SAFE_RESTART) {
                                    for (k = 0; k < MAX_DAMEON_PROGRAMS_NUMS; k++) {
                                       fprintf(log_stream, "INFO: start to restart %s safety\n", g_daemon_config->prog_list[k].program_name);
                                       if (g_prog_state_list[k].state == IS_BUSY)
                                           g_daemon_config->prog_list[k].safe_restart = true;
                                       else{
                                            process_restart(g_daemon_config->prog_list[k].program_name, g_daemon_config->prog_list[k].cmdline);
                                			g_prog_state_list[k].reboot_times++;
                                            g_prog_state_list[k].uptime = time(NULL); 
                                       }
                                    }
                                }
                                else if (control_cmd->data[0] == DAEMON_FORCE_RESTART) {
                                    for (k = 0; k < MAX_DAMEON_PROGRAMS_NUMS; k++) {
                                        fprintf(log_stream, "INFO: start to restart %s forced\n", g_daemon_config->prog_list[k].program_name);
                                        process_restart(g_daemon_config->prog_list[k].program_name, g_daemon_config->prog_list[k].cmdline);
                            			g_prog_state_list[k].reboot_times++;
                                        g_prog_state_list[k].uptime = time(NULL); 
                                    }
                                }
                                else{
                                    fprintf(log_stream, "ERROR: DAEMON TYPE is invalid\n");
                                }
                                    
                                    
				                if ((control_cmd->data[0] == DAEMON_OFF) || (control_cmd->data[0] == DAEMON_ON))
                                    fprintf(log_stream, "INFO: Control cmd:%s all programs' dameon\n", control_cmd->data[0]?"open":"close");

                            }
                            else{
                                break;
                            }
                            control_cmd_data_len += ntohs(control_cmd->len);
                            control_cmd_tmp += control_cmd_data_len;
                        }
                        
                    }
                    /* deal keepalive cmd*/
                    else if (type == TYPE_KEEPALIVE) {
                        keep_alive_t *pKeepAlive = (keep_alive_t*)(yinka_daemon_tmp + data_len);
                        program_id = ntohs(pKeepAlive->program_id);
                        time_stramp = ntohl(pKeepAlive->time_stramp);
                        state = ntohs(pKeepAlive->program_state);
                        if (program_id < YINKA_MAX){
                            g_prog_state_list[program_id-1].keepalive_failed_times = 0; 
                            g_prog_state_list[program_id-1].state = state;
                        }
                        else{
                            fprintf(log_stream, "ERROR: programid is invalid\n");
                        }
                        fprintf(log_stream, "INFO: [%s]keepalive received\n", prog_names[program_id-1]);                              
					}
                    else{
                        break;
                    }
                    data_len += value_len;
                    yinka_daemon_tmp += data_len;
                }
            }
            else {
                perror("recv");
                break;
            }       
        }
    }	
    close(g_yinka_daemon_sock);
	
    return 0;
}

static int yinka_daemon_server_init()
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

static int yinka_dameon_init()
{
    g_daemon_config = (daemon_config_t *)malloc(sizeof(daemon_config_t));
    if (!g_daemon_config) {
        return -1;
    }
	
    memset(g_daemon_config, 0, sizeof(daemon_config_t));
	memset(g_prog_state_list, 0, sizeof(program_state_t)); 
    log_stream = stderr;
    running = 0;
	
	for (int i = 0; i < MAX_DAMEON_PROGRAMS_NUMS; i++){
        memcpy(g_prog_state_list[i].prog_name, prog_names[i], strlen(prog_names[i]));
        g_prog_state_list[i].prog_name[strlen(prog_names[i])] = '\0';

    }

	yinka_daemon_server_init();
		
    /* Daemon will handle three signals */
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);

    return 0;
}
int main()
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

	running = 1;
    
	/* Never ending loop of server */
	while (running == 1) {
		process_monitor();
		process_keepalive();
		sleep(g_daemon_config->delay);
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
    
    exit(EXIT_SUCCESS);
}
