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

#ifndef MEM_CPU_INFO_H
#define MEM_CPU_INFO_H

/* Normal lines define */
#define VMRSS_LINE 21
#define PROCESS_ITEM 14

typedef struct {
	unsigned int user;
   	unsigned int nice;
	unsigned int system;
    unsigned int idle;
}sys_cpu_times;

typedef struct
{
    pid_t pid;
    unsigned int utime;
    unsigned int stime;
    unsigned int cutime;
    unsigned int cstime;
}process_cpu_times;
	
/* process physical memory occupation get function */
int process_phy_mem_get(const pid_t pid);

/* system total memory get function */
int sys_total_mem_get();

/* process memory occupation get function */
void  process_mem_rate_get(pid_t pid, long *memvalue, float *memrate);


/* process running time get function */
unsigned int process_cpu_time_get(const pid_t pid);

/* cpu total running time get function */
unsigned int sys_cpu_time_get();

/* process cpu rate get function */
float process_cpu_rate_get(pid_t pid);

#endif
