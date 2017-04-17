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

#ifndef YINKAD_H
#define YINKAD_H

#define MAX_DAMEON_PROGRAMS_NUMS        (2)
#define MAX_KEEPALIVE_FAILED_TIMES 		(10)
#define DEFAULT_DELAY   				(1)
#define YINKA_DAEMON_PORT  				(12332)

#define MAX_STR_LEN         (64)   
#define MAX_BUFFER_LEN      (512)


#define TYPE_CONTROL_CMD       (0)
#define TYPE_KEEPALIVE         (1)
#define TYPE_RES_STATISTIC     (2)
#define TYPE_XINPUT      (3)


#define IS_IDLE         (0)
#define IS_BUSY         (1)

#define XINPUT_ADD      (1)
#define XINPUT_REMOVE   (0)

#define XINPUT_DEFAULT_ENABLE_TIME (10 * 1000)



#define DEFAULT_CONF_FILE_PATH      "/etc/yinkad.conf"

char *prog_names[MAX_DAMEON_PROGRAMS_NUMS]={
						"autoprint", 
						"player"
};

#define XINPUT_ALLOW_CMDLINE ("/usr/local/bin/scripts/manage_usb_keyboard_devices.sh on")
#define XINPUT_DENY_CMDLINE ("usr/local/bin/scripts/manage_usb_keyboard_devices.sh off")

typedef struct {
    unsigned short type;
    unsigned short len;
    char data[0];
}yinka_daemon_tlv_t;

typedef struct {
    int version;
    char prog_name[MAX_STR_LEN];
    int cpurate;
    int memrate;
    long uptime;
    int reboot_times;
    int keepalive_failed_times; 
    int state;
}program_state_t;

typedef struct {
		char *cmdline;
		char *program_name;
		bool dameon_switch;
        bool safe_restart;
}program_t;

typedef struct{
	int delay;
	program_t prog_list[MAX_DAMEON_PROGRAMS_NUMS];
}daemon_config_t;


typedef struct{
    unsigned short program_id;
    int time_stramp;
    unsigned short program_state;
}keep_alive_t;

typedef struct{
    unsigned short is_enable ;
    unsigned short enable_remain_time;
}xinput_state_t;

typedef enum {
    YINKA_PRINT=1,
    YINKA_PLAYER,
    YINKA_MAX,
    YINKA_ALL=0xffff
}yinka_type;


typedef enum {
    DAEMON_OFF=0,
    DAEMON_ON,
    DAEMON_GETINFO,
    DAEMON_SAFE_RESTART,
    DAEMON_FORCE_RESTART,
    DAEMON_CLOSE,
    DAEMON_XINPUT_REPORT,
    DAEMON_XINPUT_CONTROL,   
    DAEMON_MAX
}daemon_type;


#endif
