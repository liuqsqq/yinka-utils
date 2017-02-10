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
#include <assert.h>
#include "string.h"
#include "mem_cpu_info.h"



 
int process_phy_mem_get(const pid_t pid)
{
	char file[64] = {0};
   
	FILE *fd;
	char line_buff[256] = {0};
	sprintf(file,"/proc/%d/status", pid);
 
//	fprintf (stderr, "current pid:%d\n", pid);																								   
	fd = fopen (file, "r");

	char name[32];
	int vmrss;
	 
	for (int i=0; i<VMRSS_LINE-1; i++)
	{
		fgets (line_buff, sizeof(line_buff), fd);
	}
	fgets (line_buff, sizeof(line_buff), fd);
	sscanf (line_buff, "%s %d", name, &vmrss);
	//fprintf (stderr, "====%s：%d====\n", name, vmrss);
	 
	fclose(fd);	 
	return vmrss;
} 
 
int sys_total_mem_get()
{
	char* file = "/proc/meminfo";
   
	FILE *fd;
	char line_buff[256] = {0};																					 
	char name[32];
	int memtotal;
	
	fd = fopen (file, "r");
	 
	fgets (line_buff, sizeof(line_buff), fd);
	sscanf (line_buff, "%s %d", name, &memtotal);
	//fprintf (stderr, "====%s：%d====\n", name, memtotal);
	 
	fclose(fd);
	return memtotal;
}
 
void  process_mem_rate_get(pid_t pid, long *memvalue, float *memrate)
{
	int process_mem = process_phy_mem_get(pid);
	int total = sys_total_mem_get();
    float memrate_temp = (process_mem*1.0)/(total*1.0);
    
    *memvalue = process_mem;
    *memrate = memrate_temp;
	//fprintf(stderr,"====process mem rate:%.6f\n====", memrate);
}




const char* get_items(const char* buffer,int ie)
{
	assert(buffer);
	char* p = (char *)buffer;
	int len = strlen(buffer);
	int count = 0;
	
	if (1 == ie || ie < 1)
	{
		return p;
	} 
	for (int i=0; i<len; i++)
	{
		if (' ' == *p)
		 	{
			 	count++;
			 	if (count == ie-1)
			 	{
				 	p++;
				 	break;
			 	}
		 	}
		 	p++;
	 	}
	return p;
}
unsigned int process_cpu_time_get(const pid_t pid)
{
	char file[64] = {0};//文件名
	process_cpu_times pt;
   
	FILE *fd;		   //定义文件指针fd
	char line_buff[1024] = {0};  //读取行的缓冲区
	sprintf(file,"/proc/%d/stat", pid);//文件中第11行包含着
 
//	fprintf (stderr, "current pid:%d\n", pid);																								   
	fd = fopen (file, "r"); //以R读的方式打开文件再赋给指针fd
	fgets (line_buff, sizeof(line_buff), fd); //从fd文件中读取长度为buff的字符串再存到起始地址为buff这个空间里
 
	sscanf(line_buff,"%u", &pt.pid);//取得第一项
	const char* q = get_items(line_buff, PROCESS_ITEM);//取得从第14项开始的起始指针
	sscanf(q,"%u %u %u %u", &pt.utime, &pt.stime, &pt.cutime, &pt.cstime);//格式化第14,15,16,17项
 
	//fprintf (stderr, "====pid%u:%u %u %u %u====\n", pt.pid, pt.utime, pt.stime, pt.cutime, pt.cstime);
	fclose(fd);	 //关闭文件fd
	return (pt.utime + pt.stime + pt.cutime + pt.cstime);
}
 
 
unsigned int sys_cpu_time_get()
{
	FILE *fd;		   //定义文件指针fd
	char buff[1024] = {0};  //定义局部变量buff数组为char类型大小为1024
	sys_cpu_times st;
																											  
	fd = fopen ("/proc/stat", "r"); //以R读的方式打开stat文件再赋给指针fd
	fgets (buff, sizeof(buff), fd); //从fd文件中读取长度为buff的字符串再存到起始地址为buff这个空间里
	/*下面是将buff的字符串根据参数format后转换为数据的结果存入相应的结构体参数 */
	char name[16];//暂时用来存放字符串
	sscanf (buff, "%s %u %u %u %u", name, &st.user, &st.nice, &st.system, &st.idle);
	 
	//fprintf (stderr, "====%s:%u %u %u %u====\n", name, st.user, st.nice, st.system, st.idle);
	fclose(fd);	 //关闭文件fd
	return (st.user + st.nice + st.system + st.idle);
}
 
 
float process_cpu_rate_get(pid_t pid)
{
	unsigned int totalcputime1, totalcputime2;
	unsigned int procputime1, procputime2;
	 
	totalcputime1 = sys_cpu_time_get();
	procputime1 = process_cpu_time_get(pid);
	 
	usleep(500000);//延迟500毫秒
	 
	totalcputime2 = sys_cpu_time_get();
	procputime2 = process_cpu_time_get(pid);
	 
	float cpurate = 100.0*(procputime2 - procputime1)/(totalcputime2 - totalcputime1);
	return cpurate;
}