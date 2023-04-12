#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>

#define SYSFS_ATTR_MOVEMENT	"/sys/devices/apds9960/parameters/movement"
#define SYSFS_ATTR_POWERMOD	"/sys/devices/apds9960/parameters/powermode"
#define SYSFS_ATTR_ADCITIME	"/sys/devices/apds9960/parameters/adc_itime"
#define SYSFS_ATTR_WAITTIME	"/sys/devices/apds9960/parameters/wait_time"

#define SYSFS_ATTR_PTIME	(int)(10000)
#define SYSFS_ATTR_MAXLEN	(int)(100)
#define SYSFS_ATTR_NUM		(int)(4) /* defined paths of sysfs attributes */

int main(void)
{
	int count = 0, err = 0, i = 0;
	int attr_fd[SYSFS_ATTR_NUM];
	struct pollfd ufds[SYSFS_ATTR_NUM];

	char attr_data[100];
	char attr_path[SYSFS_ATTR_NUM][SYSFS_ATTR_MAXLEN] = {
		SYSFS_ATTR_MOVEMENT,
		SYSFS_ATTR_ADCITIME,
		SYSFS_ATTR_WAITTIME,
		SYSFS_ATTR_POWERMOD
	};

	while(1) { /* if you want to stop program, use sigkill ctrl+c */
		/* Opening a connection to the attribute file */

		for(i = 0; i < SYSFS_ATTR_NUM; i++) {
			if((attr_fd[i] = open(attr_path[i], O_RDWR)) < 0) {
				printf("apds9960: open attr[%d] failed!\n", i);
				exit(1);
			}

			ufds[i].fd = attr_fd[i];
			ufds[i].events = POLLPRI | POLLERR;

			/* reading before the poll() call */
			count += read(attr_fd[i], (attr_data + count),
					(sizeof(attr_data)/sizeof(char) + 1));

			ufds[i].revents = 0;
		}

		printf("%s\n", attr_data);
		if(memset(attr_data, 0, sizeof(attr_data)) != &attr_data[0]) {
			printf("apds9960: can't clear attribte buffer!\n");
			exit(1);
		}

		if(( err = poll(ufds,  1/*SYSFS_ATTR_NUM*/, SYSFS_ATTR_PTIME)) < 0) {
			printf("apds9960: polling error!\n");
		}
		else if(err == 0) {
			printf("apds9960: polling timeout occured!\n");
		}
		else {
			printf("apds9960: polling triggered!\n");
			count = read(attr_fd[0], attr_data,
					(sizeof(attr_data)/sizeof(char) + 1));

			printf("apds9960: movement is %s\n", attr_data);
			printf("count is %d\n", count);

			/* TODO: Incorrect output for multiple attribute-polling */
			printf("movement  event: %04x\n", ufds[0].revents);
		}

		for (i = 0; i < SYSFS_ATTR_NUM; i++) {
			close(attr_fd[i]);
		}
	}

	return 0;
}
