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

#ifndef UPDATER_H_
#define UPDATER_H_
 
#define MAX_BUFFER      (1024)
#define COMMON_STR_LEN   (64)
 
#define READ_DATA_SIZE  1024  
#define MD5_SIZE        16  
#define MD5_STR_LEN     (MD5_SIZE * 2)  
 
#define DOWNLOAD_ROOT_PATH ("/tmp/updatefiles/")
#define UPDATE_FILE_NAME  ("update.tar.gz")
#define UPDATE_CONFIG_XML  ("update.xml")
#define SOFT_ROOT_PATH  ("/usr/local/soft/")
 
#define CURL_TIMEOUT (1)
 
#define LINUX_UPDATE_INTERVAL   (60 * 2)
#define MAX_RAND_SECOND         (60)

#define YINKA_DAEMON_PORT  	    (12332)
#define YINKA_LINUX_UPDATE_PORT (12333)

#define TYPE_CONTROL_CMD       0
#define TYPE_KEEPALIVE         1
#define TYPE_RES_STATISTIC     2
#define TYPE_UPDATE_CONTROL_CMD  (3)

#define DAEMON_SAFE_RESTART      (3)
 
struct _update_info_t{
    char type[COMMON_STR_LEN];
    char version[COMMON_STR_LEN];
    int update_state;
    char update_file_url[MAX_BUFFER];
    char md5[COMMON_STR_LEN]; 
}update_info_t;
 
struct _update_package_info{
    char type[COMMON_STR_LEN];    
    char update_state[COMMON_STR_LEN];
    char version[COMMON_STR_LEN];
    char timestamp[COMMON_STR_LEN];
    char author[COMMON_STR_LEN];
    char dirname[COMMON_STR_LEN];
    char cmdline[COMMON_STR_LEN]; 
}update_package_info_t;

struct _update_result_t{
    char machine_id[COMMON_STR_LEN];
    int resultCode;
    char resultDescription[COMMON_STR_LEN];
    char clientVersion[COMMON_STR_LEN];
    char starttime[COMMON_STR_LEN];
    char endtime[COMMON_STR_LEN];
}update_result_t;
 
typedef enum {
    UPDATE_PRINT=0,
    UPDATE_PLAYER,
    UPDATE_KERNEL,
    UPDATE_DEBIAN,
    UPDATE_MAX
}UPDATE_TYPE;
 
typedef enum {
    UPDATE_CODE_SUCCESS=200,
    UPDATE_CODE_DOWNLOAD_ERROR = 2000,
    UPDATE_CODE_MD5_ERROR,
    UPDATE_CODE_MAX
}UPDATE_RESULT_CODE;
 
typedef struct {
    unsigned short type;
    unsigned short len;
    char data[0];
}yinka_linux_update_tlv_t;
 
#endif
