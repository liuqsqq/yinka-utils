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
#include <syslog.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "daemon.h"


/*
 *  Callback function for handling signals.
 * 	sig	identifier of signal
 */
static void handle_signal(int sig)
{
	daemon_members members;

	if (sig == SIGINT) {
		fprintf(members.log_stream, "Debug: stopping daemon ...\n");
		/* Unlock and close lockfile */
		if (members.pid_fd != -1) {
			lockf(members.pid_fd, F_ULOCK, 0);
			close(members.pid_fd);
		}
		/* Try to delete lockfile */
		if (members.pid_file_name != NULL) {
			unlink(members.pid_file_name);
		}
		members.running = 0;
		/* Reset signal handling to default behavior */
		signal(SIGINT, SIG_DFL);
	} else if (sig == SIGHUP) {
		fprintf(members.log_stream, "Debug: reloading daemon config file ...\n");
	} else if (sig == SIGCHLD) {
		fprintf(members.log_stream, "Debug: received SIGCHLD signal\n");
	}
}

/*
 *  This function will daemonize this app
 */

static int daemonize()
{
	daemon_members members;

	pid_t pid = 0;
	int fd;
	int ret = -1;

	/* Fork off the parent process */
	pid = fork();

	/* An error occurred */
	if (pid < 0) {
		exit(EXIT_FAILURE);
		goto error;
	}

	/* Success: Let the parent terminate */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* On success: The child process becomes session leader */
	if (setsid() < 0) {
		exit(EXIT_FAILURE);
		goto error;
	}

	/* Ignore signal sent from child to parent process */
	signal(SIGCHLD, SIG_IGN);

	/* Fork off for the second time*/
	pid = fork();

	/* An error occurred */
	if (pid < 0) {
		exit(EXIT_FAILURE);
		goto error;
	}

	/* Success: Let the parent terminate */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* Set new file permissions */
	umask(0);

	/* Change the working directory to the root directory */
	/* or another appropriated directory */
	chdir("/");

	/* Close all open file descriptors */
	for (fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--) {
		close(fd);
	}

	/* Reopen stdin (fd = 0), stdout (fd = 1), stderr (fd = 2) */
	stdin = fopen("/dev/null", "r");
	stdout = fopen("/dev/null", "w+");
	stderr = fopen("/dev/null", "w+");

	/* Try to write PID of daemon to lockfile */
	if (members.pid_file_name != NULL)
	{
		char str[256];
		members.pid_fd = open(members.pid_file_name, O_RDWR|O_CREAT, 0640);
		if (members.pid_fd < 0) {
			/* Can't open lockfile */
			exit(EXIT_FAILURE);
			goto error;
		}
		if (lockf(members.pid_fd, F_TLOCK, 0) < 0) {
			/* Can't lock file */
			exit(EXIT_FAILURE);
			goto error;
		}
		/* Get current PID */
		sprintf(str, "%d\n", getpid());
		/* Write PID to lockfile */
		write(members.pid_fd, str, strlen(str));

		return 0;
	}
error:
	return ret;
}

/*
 * Print help for this application
 */
static void print_help()
{
	daemon_members members;
	
	printf("\n Usage: %s [OPTIONS]\n\n", members.app_name);
	printf("  Options:\n");
	printf("   -h --help                 Print this help\n");
	printf("   -c --conf_file filename   Read configuration from the file\n");
	printf("   -l --log_file  filename   Write logs to the file\n");
	printf("   -d --daemon               Daemonize this application\n");
	printf("   -p --pid_file  filename   PID file used by daemonized app\n");
	printf("\n");
}

/* Main function */
int main(int argc, char *argv[])
{
	daemon_members members;
	int ret, value;
	int option_index = 0;
	int counter = 0;

	/* Daemon members init */
	members.log_file_name = NULL;
	members.pid_file_name = NULL;
	members.app_name = NULL;
	members.pid_fd = -1;
	members.running = 0;

	members.app_name = argv[0];

	/* Try to process all command line arguments */
	while ((value = getopt_long(argc, argv, "l:p:dh", daemon_options, &option_index)) != -1) {
		switch (value) {
			case 'l':
				members.log_file_name = strdup(optarg);
				break;
			case 'p':
				members.pid_file_name = strdup(optarg);
				break;
			case 'd':
				members.start_daemonized = 1;
				break;
			case 'h':
				print_help();
				return EXIT_SUCCESS;
			default:
				break;
		}
	}

	/* When daemonizing is requested at command line. */
	if (members.start_daemonized == 1) {
		/* It is also possible to use glibc function deamon()
		 * at this point, but it is useful to customize your daemon. */
		ret = daemonize();
		if (ret != 0)
			syslog(LOG_ERR, "Can not init the daemon program: %s, error: %s\n", 
				members.log_file_name, strerror(errno));
	}

	/* Open system log and write message to it */
	openlog(argv[0], LOG_PID|LOG_CONS, LOG_DAEMON);
	syslog(LOG_INFO, "Started %s", members.app_name);

	/* Daemon will handle two signals */
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);

	/* Try to open log file to this daemon */
	if (members.log_file_name != NULL) {
		members.log_stream = fopen(members.log_file_name, "a+");
		if (members.log_stream == NULL) {
			syslog(LOG_ERR, "Can not open log file: %s, error: %s",
				members.log_file_name, strerror(errno));
			members.log_stream = stdout;
		}
	} else {
		members.log_stream = stdout;
	}

	/* This global variable can be changed in function handling signal */
	members.running = 1;

	/* Never ending loop of server */
	while (members.running == 1) {
		/* Debug print */
		ret = fprintf(members.log_stream, "Debug: %d\n", counter++);
		if (ret < 0) {
			syslog(LOG_ERR, "Can not write to log stream: %s, error: %s",
				(members.log_stream == stdout) ? "stdout" : members.log_file_name, strerror(errno));
			break;
		}
		ret = fflush(members.log_stream);
		if (ret != 0) {
			syslog(LOG_ERR, "Can not fflush() log stream: %s, error: %s",
				(members.log_stream == stdout) ? "stdout" : members.log_file_name, strerror(errno));
			break;
		}

		/* TODO: dome something useful here */

		/* Real server should use select() or poll() for waiting at
		 * asynchronous event. Note: sleep() is interrupted, when
		 * signal is received. */
		sleep(10);
	}

	/* Close log file, when it is used. */
	if (members.log_stream != stdout) {
		fclose(members.log_stream);
	}

	/* Write system log and close it. */
	syslog(LOG_INFO, "Stopped %s", members.app_name);
	closelog();

	/* Free allocated memory */
	if (members.log_file_name != NULL) free(members.log_file_name);
	if (members.pid_file_name != NULL) free(members.pid_file_name);

	return EXIT_SUCCESS;
}

