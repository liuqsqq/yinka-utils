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


/* define a struct contain useful members */
typedef struct daemon_members{
	char *log_file_name;
	char *pid_file_name;
	int start_daemonized;
	int pid_fd;
	char *app_name;
	int running;
	FILE *log_stream;
} daemon_members;

/* Use this function to start a daemon */
int daemon_start(int argc, char *argv[]);

/* Use this function to stop a daemon */
int daemon_stop();

			
