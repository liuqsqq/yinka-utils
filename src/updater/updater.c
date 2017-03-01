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
#include <string.h>
#include <unistd.h>
#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>

#include <curl/curl.h>
#include <json-c/json.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <pthread.h>
#include <sys/socket.h> 
#include <sys/un.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "../utils/md5.h"
#include "updater.h"
 
const char update_request_url[MAX_BUFFER] = {"http://admin.yinka.co/index.php?r=interface/printor/GetUpdateInfoById\&machine_id="};
const char update_report_url[MAX_BUFFER] = {"http://admin.yinka.co/index.php?r=interface/printor/ReportUpdateResult"};
static const char *items_version_name[] = {"autoprint_version.ver", "player_version.ver", "kernel_version.ver", "debian_verison.ver"};

static struct _update_info_t update_str_info = {0};
static struct _update_package_info update_packages_info[UPDATE_MAX] = {0};

static int g_yinka_linux_update_sock = -1;
static FILE *log_stream;

int yinka_linux_update_server_init();
int  process_linux_update_cmd_data(char *ptr);

/* write update request info */
size_t write_request_data(void * ptr, size_t size, size_t nmemb, void* stream)  
{  
    strcpy(stream, ptr);  
    //printf("received  %dBytes request data\n", size * nmemb);
    return (size * nmemb);  
}  

/* download update.tar.gz */
size_t write_update_data(void * ptr, size_t size, size_t nmemb, void* stream)  
{
    int write_bytes = 0;
    write_bytes = fwrite(ptr, size, nmemb, (FILE *)stream);
    //printf("write_update_data:%d\n", write_bytes);
    return write_bytes;  
}  

/* write report request info */
size_t write_report_data(void * ptr, size_t size, size_t nmemb, void* stream)  
{  
    strcpy(stream, ptr);  
    //printf("received  %dBytes report data\n", size * nmemb);
    return (size * nmemb); 
}

int parse_update_info(char* str_info)
{
    json_object *json_update_info = NULL;
    json_object * temp = NULL;
    json_bool ret;
    const char* pstr = NULL;
    json_update_info = json_tokener_parse(str_info);
    
    ret = json_object_object_get_ex(json_update_info, "updatestate", &temp);
    if (ret == TRUE) {    
        //printf("updatestate = %d\n", json_object_get_int(temp)); 
        update_str_info.update_state = json_object_get_int(temp);
    }
    else {
        goto final;
    }
    
    ret = json_object_object_get_ex(json_update_info, "type", &temp);
    if (ret == TRUE) {
        pstr = json_object_get_string(temp);
        //printf("type = %s\n", pstr);   
        //strcpy(update_str_info.type, pstr);
        memcpy(update_str_info.type, pstr, strlen(pstr));
        update_str_info.type[strlen(pstr)] = '\0';
    }

    ret = json_object_object_get_ex(json_update_info, "version", &temp);
    if (ret == TRUE) {
        pstr = json_object_get_string(temp);
        //printf("version = %s\n", pstr);  
        //strcpy(update_str_info.type, pstr);
        memcpy(update_str_info.version, pstr, strlen(pstr));
        update_str_info.version[strlen(pstr)] = '\0';  
    }
    
    ret = json_object_object_get_ex(json_update_info, "md5", &temp);
    if (ret == TRUE) {
        pstr = json_object_get_string(temp);
        //printf("md5 = %s\n", pstr);   
        memcpy(update_str_info.md5, pstr, strlen(pstr));
        update_str_info.md5[strlen(pstr)] = '\0'; 
    }
    
    ret = json_object_object_get_ex(json_update_info, "url", &temp);
    if (ret == TRUE) {
        pstr = json_object_get_string(temp);
        //printf("url = %s\n", pstr);   
        memcpy(update_str_info.update_file_url, pstr, strlen(pstr));
        update_str_info.update_file_url[strlen(pstr)] = '\0'; 
    }
 
final:
    json_object_put(json_update_info);
}

int make_json_paras(struct _update_result_t result, char* result_json)
{
    json_object *json=json_object_new_object();

    json_object_object_add(json, "machine_id",json_object_new_string(result.machine_id));
    json_object_object_add(json, "resultCode",json_object_new_int(result.resultCode));
    json_object_object_add(json, "resultDescription",json_object_new_string(result.resultDescription));
    json_object_object_add(json, "clientVersion",json_object_new_string(result.clientVersion));
    json_object_object_add(json, "starttime",json_object_new_string(result.starttime));
    json_object_object_add(json, "endtime",json_object_new_string(result.endtime));
    
    const char *str=json_object_to_json_string(json);
    //printf("report json is %s\n",str);
    strcpy(result_json, str);
    json_object_put(json);

    return 0;
}
 
int query_update_info(const char* machine_id)
{
    CURL *curl;
    CURLcode res;
    long retcode = 0;
    char update_info[MAX_BUFFER] = {0};
    char server_url_full[MAX_BUFFER] ={0};
    
    if (machine_id == NULL)
        return -1;
        //memset(update_info, 0 ,sizeof(update_info));
        memset(server_url_full, 0, sizeof(server_url_full));
    /* In windows, this will init the winsock stuff */ 

    /* get a curl handle */ 
    curl = curl_easy_init();
    if(curl) {
        strcat(server_url_full, update_request_url);
        strcat(server_url_full, machine_id);
        server_url_full[strlen(server_url_full)] = '\0';
     
        curl_easy_setopt(curl, CURLOPT_URL, server_url_full);    
     
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_request_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, update_info);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT);

        /* Perform the request, res will get the return code */ 
        res = curl_easy_perform(curl);
        /* Check for errors */ 
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
            return -1;
        }
     
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &retcode);

        if ( (res == CURLE_OK) && retcode == 200 ){
            //printf("update_info : %s", update_info);
            parse_update_info(update_info);       
        }
        else {
            fprintf(stderr, "retcode is %ld \n",retcode); 
        }

    /* always cleanup */ 
    curl_easy_cleanup(curl);
    }
    
    return retcode;
}

int download_update_file(char* file_path_url)
{
    CURL *curl;
    CURLcode return_code; 
    long retcode = 0;
    char file_path[COMMON_STR_LEN] = {0};

    if (file_path_url == NULL)
        return -1;

    strcat(file_path, DOWNLOAD_ROOT_PATH);
    strcat(file_path, UPDATE_FILE_NAME);

    if (access(file_path, F_OK) != -1){
        #if 0
            strcat(file_name, "rm -rf ");
            strcat(file_name, UPDATE_FILE_NAME);
            system(file_name);
        #endif
        remove(file_path);
        fprintf(stderr, "%s is exist, rm it\n", file_path);  
    }
    
    FILE *fp = fopen(file_path, "ab+");
    if (fp == NULL)
        return -1;

    /* get a curl handle */ 
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, file_path_url);  
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_update_data);  
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, TRUE); 

        /* Perform the request, res will get the return code */ 
        return_code = curl_easy_perform(curl);
     
        /* Check for errors */ 
        if(return_code != CURLE_OK){
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(return_code));
            return -1;
        }
         
        return_code = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &retcode);
        if ( (return_code == CURLE_OK) && retcode == UPDATE_CODE_SUCCESS ) {
            double file_size = 0.0;  
            return_code = curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD , &file_size);  
            if((CURLE_OK==return_code) && file_size)  
                printf("file_size: %0.3fMB.\n", file_size/1024/1024);
            double downloadSpeed = 0.0;  
            return_code = curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD , &downloadSpeed);  
            if((CURLE_OK==return_code) && downloadSpeed)  
                printf("speed: %0.3f kb/s.\n", downloadSpeed/1024);    
        }
        else {
            printf("retcode == %ld\n", retcode);
        }
     /* always cleanup */ 
     curl_easy_cleanup(curl);        
    }
    
    fclose(fp); 
    return 0;
}
 
int report_update_result(const char* machine_id, char* para_json_str)
{
    CURL *curl;
    CURLcode res;
    long retcode = 0;
    char report_info[MAX_BUFFER] = {0};
    char server_url_full[MAX_BUFFER] ={0};
    if (machine_id == NULL)
     return -1;

    /* get a curl handle */ 
    curl = curl_easy_init();
    if(curl) {
        strcat(server_url_full, update_report_url);
        // strcat(server_url_full, machine_id);
        // server_url_full[strlen(server_url_full)] = '\0';
         
        curl_easy_setopt(curl, CURLOPT_URL, server_url_full);         
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_report_data);  
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, report_info); 
        // 设置http发送的内容类型为JSON  
        struct curl_slist *plist = curl_slist_append(NULL, "Content-Type:application/json;charset=UTF-8");  
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, plist);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        #if 0
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        #endif
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT);//设置超时

        /* Now specify the POST data */ 
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, para_json_str);
        
        /* Perform the request, res will get the return code */ 
        res = curl_easy_perform(curl);
         
        /* Check for errors */ 
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
            return -1;
        }

        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &retcode);
        if ( (res == CURLE_OK) && retcode == 200 ) {             
            //printf("report_info : %s", report_info);
            //fprintf(stderr, "report ok,retcode is %ld \n",retcode); 
        }
        else {
           fprintf(stderr, "retcode is %ld \n",retcode); 
        }
        /* always cleanup */ 
        curl_easy_cleanup(curl);
     }

     return retcode;
}
 
int compute_file_md5(const char *file_path, char *md5_str)  
{  
    int i;  
    int fd;  
    int ret;  
    unsigned char data[READ_DATA_SIZE];  
    unsigned char md5_value[MD5_SIZE];  
    MD5_CTX md5;  

    fd = open(file_path, O_RDONLY);  
    if (-1 == fd) {
        perror("open");  
        return -1;  
    }  

    // init md5  
    MD5Init(&md5);  

    while (1) {  
        ret = read(fd, data, READ_DATA_SIZE);  
        if (-1 == ret) {  
            perror("read");  
            return -1;  
        }  

        MD5Update(&md5, data, ret);  

        if (0 == ret || ret < READ_DATA_SIZE) {  
            break;  
        }  
    }  

    close(fd);  

    MD5Final(&md5, md5_value);  

    for(i = 0; i < MD5_SIZE; i++) {  
        snprintf(md5_str + i*2, 2+1, "%02x", md5_value[i]);  
    }  
        md5_str[MD5_STR_LEN] = '\0'; // add end  

    return 0;  
}  

void store_update_packages_info(xmlNodePtr pcur, UPDATE_TYPE type)
{
    xmlNodePtr nptr=pcur->xmlChildrenNode;
    char *pstr = NULL;
    while (nptr != NULL) {
        pstr = ((char*)XML_GET_CONTENT(nptr->xmlChildrenNode));
        if (!xmlStrcmp(nptr->name, BAD_CAST("updatestate"))) {            
            //printf("updatestate: %s\n", pstr);
            strcpy(update_packages_info[type].update_state, pstr);
        }
        else if(!xmlStrcmp(nptr->name, BAD_CAST("version"))) {
            //printf("version: %s\n",pstr); 
            strcpy(update_packages_info[type].version, pstr);
        }
        else if(!xmlStrcmp(nptr->name, BAD_CAST("timestamp"))) {
            //printf("timestamp: %s\n",pstr);
            strcpy(update_packages_info[type].timestamp, pstr);                 
        }
        else if(!xmlStrcmp(nptr->name, BAD_CAST("author"))) {
            //printf("author: %s\n",pstr);
            strcpy(update_packages_info[type].author, pstr);                    
        }
        else if(!xmlStrcmp(nptr->name, BAD_CAST("dirname"))) {
            //printf("dirname: %s\n",pstr);
            strcpy(update_packages_info[type].dirname, pstr);                     
        }
        else if(!xmlStrcmp(nptr->name, BAD_CAST("updatecmd"))) {
            //printf("updatecmd: %s\n",pstr);
            strcpy(update_packages_info[type].cmdline, pstr);                    
        }
        nptr = nptr->next;
    }    
}
int parse_update_xml (char* xml_path)
{
    UPDATE_TYPE  type = UPDATE_MAX;
    xmlDocPtr pdoc = NULL;
    xmlNodePtr proot = NULL, pcur = NULL;
    /*****************打开xml文档********************/
    xmlKeepBlanksDefault(0);//必须加上，防止程序把元素前后的空白文本符号当作一个node
    pdoc = xmlReadFile (xml_path, "UTF-8", XML_PARSE_RECOVER);//libxml只能解析UTF-8格式数据

    if (pdoc == NULL) {
        printf ("error:can't open file!\n");
        return -1;
    }

    /*****************获取xml文档对象的根节对象********************/
    proot = xmlDocGetRootElement (pdoc);

    if (proot == NULL) {
        printf("error: file is empty!\n");
        return -1;
    }

    /*****************查找所有的子节点********************/
    pcur = proot->xmlChildrenNode;

    while (pcur != NULL) {
        //如同标准C中的char类型一样，xmlChar也有动态内存分配，字符串操作等 相关函数。例如xmlMalloc是动态分配内存的函数；xmlFree是配套的释放内存函数；xmlStrcmp是字符串比较函数等。
        //对于char* ch="book", xmlChar* xch=BAD_CAST(ch)或者xmlChar* xch=(const xmlChar *)(ch)
        //对于xmlChar* xch=BAD_CAST("book")，char* ch=(char *)(xch)
        if (!xmlStrcmp(pcur->name, BAD_CAST("print"))) {
            type = UPDATE_PRINT;
        }
        else if (!xmlStrcmp(pcur->name, BAD_CAST("player"))) {
            type = UPDATE_PLAYER;
        }  
        else if (!xmlStrcmp(pcur->name, BAD_CAST("kernel"))) {
            type = UPDATE_KERNEL;
        }    
        else if (!xmlStrcmp(pcur->name, BAD_CAST("debian"))) {
            type = UPDATE_DEBIAN;
        }
        if (type < UPDATE_MAX)
            store_update_packages_info(pcur, type);
        pcur = pcur->next;
    }

    /*****************释放资源********************/
    xmlFreeDoc (pdoc);
    xmlCleanupParser ();
    xmlMemoryDump ();
    
    return 0;
}

static int local_version_get(UPDATE_TYPE type, char *version)
{
    FILE *fp = NULL;
    char file[COMMON_STR_LEN] = {0};
    char line_buff[COMMON_STR_LEN] = {0};

    strcat(file, SOFT_ROOT_PATH);
    strcat(file, items_version_name[type]);
 
    fp = fopen (file, "r");
    if (fp == NULL) {
        fprintf(stderr, "open %s failed\n", file);
        return -1;
    }
    if(fgets (line_buff, sizeof(line_buff), fp)!= NULL) {
        sscanf (line_buff, "%s", version);    
    }
    else {
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    return 0;
}

static void local_time_get(char* local_time)
{
    char szBuf[COMMON_STR_LEN] = {0};  
    time_t timer = time(NULL);  
    strftime(szBuf, sizeof(szBuf), "%Y-%m-%d %H:%M:%S", localtime(&timer));  
    strcpy(local_time, szBuf);
    //printf("%s\n", local_time);  
}

int machine_id_get(char* UUID, char* machine_id)
{
    if ((UUID == NULL) || (machine_id == NULL))
        return -1;
    strcpy(machine_id, UUID);
    
    return 0;
}

int rand_time_get(int rand_range)
{
    srand((unsigned)time(NULL)); //用于保证是随机数
    return (rand() % rand_range);  //用rand产生随机数并设定范围
}

int  yinka_linux_update_init()
{
    g_yinka_linux_update_sock = -1;
    log_stream = stderr;
    yinka_linux_update_server_init();
}

int  yinka_linux_update_reset()
{
    char tmp_file_path[COMMON_STR_LEN] = {0};
    int ret = 0;
    
    memset(&update_str_info, 0, sizeof(update_str_info));
    memset(&update_packages_info, 0, sizeof(update_packages_info));
    if (access(DOWNLOAD_ROOT_PATH, F_OK) != -1) {
        printf("update start, force to delete /tmp/updatefiles/\n");
        memset(tmp_file_path, 0 ,sizeof(tmp_file_path));
        /* force to delete /tmp/updatefiles/ */
        strcat(tmp_file_path, "rm -rf ");
        strcat(tmp_file_path, DOWNLOAD_ROOT_PATH);
        system(tmp_file_path);
    }
    
    ret = mkdir(DOWNLOAD_ROOT_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (ret != 0 ) {
        fprintf(log_stream, "create updatefiles path failed, exit\n");
        return -1;
    }
    fprintf(log_stream, "/tmp/updatefiles path is not exist, create it\n");
    
    return 0;  
}

int process_update(char* machine_id,  int force_update_flag)
{
    int ret = 0;
    char str_md5[COMMON_STR_LEN] = {0};
    char str_tarcmd[MAX_BUFFER] = {0};  
    char update_file_path[COMMON_STR_LEN] = {0};
    char tmp_file_path[COMMON_STR_LEN] = {0};
    char version[COMMON_STR_LEN]= {0};
    char local_time[COMMON_STR_LEN] = {0};
    char result_json[MAX_BUFFER] = {0};
    int force_update = 0;
    struct _update_result_t update_result;
 
    strcat(update_file_path, DOWNLOAD_ROOT_PATH);
    strcat(update_file_path, UPDATE_FILE_NAME);

    strcat(str_tarcmd, "tar xzf ");
    strcat(str_tarcmd, update_file_path);
    strcat(str_tarcmd, " -C ");
    strcat(str_tarcmd, DOWNLOAD_ROOT_PATH);


    memset(tmp_file_path, 0 ,sizeof(tmp_file_path));
    memset(&update_result, 0, sizeof(update_result));
    memset(version, 0, sizeof(version));
    memset(&local_time, 0, sizeof(local_time));   
    yinka_linux_update_reset();  

    force_update = force_update_flag;    
    strcpy(update_result.machine_id, machine_id);   
    local_time_get(local_time);
    strcpy(update_result.starttime, local_time);
    memset(local_time, 0, sizeof(local_time));

 
    /* todo...we need get machine_id by a print_identifier 
    and, assure that network is well done between local and server point */
    ret = query_update_info(machine_id);
    if ((ret == -1))
        return -1;    
    #if 0            
        printf("updatestate = %d\n", update_str_info.update_state); 
        printf("type = %s\n", update_str_info.type);   
        printf("version = %s\n", update_str_info.version);   
        printf("url = %s\n", update_str_info.update_file_url); 
        printf("md5 = %s\n", update_str_info.md5);        
    #endif
 

    /* if force_update is valid or update_state is invalid, try to update*/
    if ((force_update == 1) || (update_str_info.update_state != UPDATE_CODE_SUCCESS)){
        printf("start to download update.tar.gz\n");
        ret = download_update_file(update_str_info.update_file_url);
        if (ret != -1){
            printf("download ok, start to check update.tar.gz md5\n");
            ret = compute_file_md5(update_file_path,str_md5);    
            if (ret != -1){
                if (strcmp(str_md5, update_str_info.md5) == 0){
                    printf("md5 check pass,start to tar\n");                      
                    system(str_tarcmd);  
                 
                    strcat(tmp_file_path, DOWNLOAD_ROOT_PATH);
                    strcat(tmp_file_path, UPDATE_CONFIG_XML);
                    printf("tar ok, start to parse update.xml\n");
                    
                    ret = parse_update_xml(tmp_file_path);
                    if (ret != -1){
                        memset(tmp_file_path, 0 ,sizeof(tmp_file_path));
                        printf("parse ok, start to execute coresponding cmdline\n");
                        for (int i = 0; i < UPDATE_MAX; i++) {
                            #if 0
                                 printf("this is %d as follows:\n", i);
                                 printf("updatestate: %s\n", update_packages_info[i].update_state); 
                                 printf("timestamp: %s\n", update_packages_info[i].timestamp);  
                                 printf("author: %s\n", update_packages_info[i].author); 
                                 printf("dirname: %s\n", update_packages_info[i].dirname);   
                                 printf("cmdline: %s\n", update_packages_info[i].cmdline); 
                            #endif
 
                            /* if update_state is true, execute its cmdline*/
                            if (!strcmp("true", update_packages_info[i].update_state)) {
                                local_version_get(i, version);
                                fprintf(stderr, "version %s\n", version);                                
                                if ((force_update != 1) && (!strcmp(version, update_packages_info[i].version)))
                                    continue;
                             
                                strcat(tmp_file_path, "echo \"cqutprint\" | sudo -S ");
                                strcat(tmp_file_path, update_packages_info[i].cmdline);
                                fprintf(stderr, "start to execute %s\n", tmp_file_path);
                                system(tmp_file_path);
                                memset(tmp_file_path, 0 ,sizeof(tmp_file_path));
                                #if 0
                                    fprintf(stderr, "start to execute %s\n", update_packages_info[i].cmdline);
                                    system(update_packages_info[i].cmdline);
                                #endif
                            }
                        } 
                    }
                    printf("update compete, force to delete /tmp/updatefiles/\n");
                    memset(tmp_file_path, 0 ,sizeof(tmp_file_path));
                    #if 0
                        /* force to delete /tmp/updatefiles/ */
                        strcat(tmp_file_path, "rm -rf ");
                        strcat(tmp_file_path, DOWNLOAD_ROOT_PATH);
                        system(tmp_file_path);
                    #endif
                    update_result.resultCode = UPDATE_CODE_SUCCESS;
                    strcpy(update_result.resultDescription, "update compete successful");
                    strcpy(update_result.clientVersion, update_str_info.version);
                    local_time_get(local_time);
                    strcpy(update_result.endtime, local_time);
                }
                else {
                    update_result.resultCode = UPDATE_CODE_MD5_ERROR;
                    strcpy(update_result.resultDescription, "update failed, md5 check faield");
                    local_time_get(local_time);
                    strcpy(update_result.endtime, local_time);                        
                    fprintf(stderr, "file md5 is not match,update faield\n");
                }
            }
            else {
                update_result.resultCode = UPDATE_CODE_MD5_ERROR;
                strcpy(update_result.resultDescription, "update failed, computer md5  faield");
                local_time_get(local_time);
                strcpy(update_result.endtime, local_time); 
                fprintf(stderr, "compute md5 error,update faield\n");
            }        
        }
        else {
            update_result.resultCode = UPDATE_CODE_DOWNLOAD_ERROR;
            strcpy(update_result.resultDescription, "update failed, download update file faield");
            local_time_get(local_time);
            strcpy(update_result.endtime, local_time); 
            fprintf(stderr, "download_update_file failed\n");
        }
        ret = make_json_paras(update_result, result_json);
        ret = report_update_result(machine_id, result_json);
        if (ret != -1) {
            //fprintf(stderr, "report resultcod = %d\n", ret);
        }
    }
    else {
        printf("needn't update\n");
    }
}

int main(void)
{
    int ret = 0;
    int rand_sec = 0;
    char machine_id[COMMON_STR_LEN] = {0};
 
    curl_global_init(CURL_GLOBAL_ALL);

    ret = yinka_linux_update_init();
 
    ret = machine_id_get("CQUT-FABU", machine_id);
 
    for(;;){
        rand_sec = rand_time_get(MAX_RAND_SECOND);
        sleep(LINUX_UPDATE_INTERVAL + rand_sec);
        process_update(machine_id, 0);
    }  
    curl_global_cleanup();
    
    return 0;
} 

int yinka_linux_update_server_init()
{
    pthread_t  yinka_daemon_thread;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(YINKA_LINUX_UPDATE_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if ( (g_yinka_linux_update_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    // 设置套接字选项避免地址使用错误  
    int on=1;  
    if((setsockopt(g_yinka_linux_update_sock,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)))<0) {  
        perror("setsockopt failed");  
        exit(EXIT_FAILURE);  
    }  
 
    if (bind(g_yinka_linux_update_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    } 
 
    pthread_create(&yinka_daemon_thread, NULL, (void *)(&process_linux_update_cmd_data), NULL); 
    return 0;
}

int process_linux_update_cmd_data(char *ptr)
{
    char buff[MAX_BUFFER] = {0};
    char * p_yinka_update_temp= NULL;
    yinka_linux_update_tlv_t *p_yinka_update = NULL;  
    fd_set set;
    struct timeval timeout;    
    struct sockaddr_in clientAddr;
    int recv_bytes;
    int max_fd;
    int nfound;
    int type;
    int value_len;
    int clientAddrlen = sizeof(clientAddr);  
    int force_update = 0;
    int ret = 0;
    char machine_id[COMMON_STR_LEN] = {0};
 
    while (1) {
        FD_ZERO(&set);
        FD_SET(g_yinka_linux_update_sock, &set);
        max_fd  = g_yinka_linux_update_sock;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        memset(buff, 0 , sizeof(buff));
        nfound = select(max_fd + 1, &set, (fd_set *)0, (fd_set *)0, &timeout);
        if(nfound  < 0) {
            fprintf(log_stream, "ERR: select error!\n");
            continue;
        }
     
    #if 0
        else if (nfound == 0)
        {
            fprintf(log_stream, "\nDebug: select timeout!");
            continue;
        }
    #endif 
     
        if (FD_ISSET(g_yinka_linux_update_sock, &set)) {        
            recv_bytes = recvfrom(g_yinka_linux_update_sock, buff, MAX_BUFFER, 0, (struct sockaddr*)&clientAddr, &clientAddrlen);
            if (recv_bytes > 0) {
                buff[recv_bytes] = 0;
                //fprintf(log_stream,"INFO:Receive %d bytes\n", recv_bytes); 
                p_yinka_update_temp = buff;    

                p_yinka_update = (yinka_linux_update_tlv_t *)p_yinka_update_temp;
                type = ntohs(p_yinka_update->type);
                value_len = ntohs(p_yinka_update->len);
             
                #if 0
                    fprintf(log_stream, "INFO:type:%d,len=%d\n", type, value_len); 
                    fprintf(log_stream, "INFO:value:\n");
                    for (int i = 0; i < value_len; i++) {
                        fprintf(log_stream, "%2x ", p_yinka_update->data[i]); 
                    }
                #endif
             
                /* deal control cmd*/
                if (type == TYPE_UPDATE_CONTROL_CMD) {
                    fprintf(log_stream, "INFO:force to update reset now\n");
                    force_update = p_yinka_update->data[0];   
                    ret = machine_id_get("CQUT-FABU", machine_id);
                    ret = process_update(machine_id, force_update);
                    memset(machine_id, 0, sizeof(machine_id));
                }
            }
            else {
                fprintf(log_stream, "ERR:Receive error\n"); 
                break;
            }       
        }
    }
    close(g_yinka_linux_update_sock);
    
    return 0;
}
