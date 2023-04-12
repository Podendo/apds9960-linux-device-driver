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
#define SYSFS_ATTR_MAXLEN	(int)(50)
#define SYSFS_ATTR_NUM		(int)(4) /* defined paths of sysfs attributes */

/**
 * @brief Reads / Prints a sysfs sensor-attributes according to received event
 * @param	fd		An array of attribute's file-descriptors
 * @param	buf		Data-buffer for each attribute's value
 * @param	ufds		array of file-descriptor for polling call
 * @param	attr_num	Number of sysfs-nodes for polling call
 */
int read_attributes(int *fd, void *buf, void *ufds, int attr_num)
{
	int i = 0, err = 1;
	struct pollfd *priv = (struct pollfd *)ufds;

	for( i = 0; i < attr_num; i++) {
		if(priv[i].revents == (POLLPRI | POLLERR)) {
			err *= pread(fd[i], buf, SYSFS_ATTR_MAXLEN, 0);
			printf("parameter %d is %s", i, (char*)buf);
			printf("revents %d is %04x\n", i, priv[i].revents);
		} else {
			__asm__("nop");
		}
	}

	return err;
}

int main(void)
{
	int err = 0, i = 0;
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

			/* dummy-reading before the poll() call */
			if( ! pread(attr_fd[i], (attr_data),
					(sizeof(attr_data)/sizeof(char)), 0)) {
				printf("apds9960: 0 bytes from %d node!\n", i);
				exit(1);
			}

			ufds[i].revents = 0;
			/*printf("%s\n", attr_data); print dummy reads for debug */
		}

		if(( err = poll(ufds, SYSFS_ATTR_NUM, SYSFS_ATTR_PTIME)) < 0) {
			printf("apds9960: polling error!\n");
		}
		else if(err == 0) {
			printf("apds9960: polling timeout occured!\n");
		}
		else {
			printf("apds9960: polling triggered!\n");
			if(read_attributes(attr_fd, attr_data,
						ufds, SYSFS_ATTR_NUM) == 0) {
				printf("apds9960: can`t read attribues!\n");
				exit(1);
			}
		}

		for (i = 0; i < SYSFS_ATTR_NUM; i++) {
			close(attr_fd[i]);
		}
	}

	return 0;
}
