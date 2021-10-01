/*
 * Copyright (c) 2021 joshua stein <jcs@jcs.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <vis.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>

static int debug = 0;
static char vbuf[512];
static int serial_fd = -1;

void usage(const char *);
void serial_setup(int, speed_t);
size_t serial_read(void *, size_t);
size_t serial_write(const void *, size_t);

void
usage(const char *progname)
{
	fprintf(stderr, "usage: %s [-d] [-s serial speed] <serial device>\n",
	    progname);
	exit(1);
}

void
serial_setup(int fd, speed_t speed)
{
	struct termios tty;

	if (ioctl(fd, TIOCEXCL) != 0)
		err(1, "ioctl(TIOCEXCL)");
	if (tcgetattr(fd, &tty) < 0)
		err(1, "tcgetattr");

	tty.c_cflag |= CREAD;
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;			/* 8-bit characters */
	tty.c_cflag &= ~PARENB;			/* no parity bit */
	tty.c_cflag &= ~CSTOPB;			/* only need 1 stop bit */

	/* setup for non-canonical mode */
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty.c_oflag &= ~OPOST;

	/* fetch bytes as they become available */
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 1;

	cfsetspeed(&tty, speed);

	if (tcsetattr(fd, TCSAFLUSH, &tty) != 0)
		err(1, "tcsetattr");
}

size_t
serial_read(void *buf, size_t nbytes)
{
	size_t ret;

	ret = read(serial_fd, buf, nbytes);

	if (debug && ret > 0) {
		strvisx(vbuf, buf, ret, VIS_NL | VIS_CSTYLE | VIS_OCTAL);
		printf("<<< %s\n", vbuf);
	}

	return ret;
}

size_t
serial_write(const void *buf, size_t nbytes)
{
	if (debug) {
		strvisx(vbuf, buf, nbytes, VIS_NL | VIS_CSTYLE | VIS_OCTAL);
		printf(">>> %s\n", vbuf);
	}

	return write(serial_fd, buf, nbytes);
}

int
main(int argc, char *argv[])
{
	struct timeval tp;
	struct tm *tm;
	char *serial_dev = NULL;
	char buf[64];
	int serial_speed = B1200;
	int ch;

	if (argc == 1)
		usage(argv[0]);

	while ((ch = getopt(argc, argv, "ds:")) != -1) {
		switch (ch) {
		case 'd':
			debug = 1;
			break;
		case 's':
			serial_speed = (unsigned)strtol(optarg, NULL, 0);
			if (errno)
				err(1, "invalid serial port speed value");
			break;
		default:
			usage(argv[0]);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage(argv[0]);

	serial_dev = argv[0];
	serial_fd = open(serial_dev, O_RDWR|O_SYNC);
	if (serial_fd < 0)
		err(1, "open %s", serial_dev);

	serial_setup(serial_fd, serial_speed);

	for (;;) {
		gettimeofday(&tp, NULL);
		tm = localtime(&tp.tv_sec);
		if (debug)
			printf("currently: %02d:%02d:%02d\n", tm->tm_hour,
			    tm->tm_min, tm->tm_sec);
		if (tm->tm_sec == 0) {
			if (debug)
				printf("setting chronograph to %02d%02d00\n",
				    tm->tm_hour, tm->tm_min);
			sprintf(buf, "\rATST%02d%02d00\r", tm->tm_hour,
			    tm->tm_min);
			serial_write(buf, strlen(buf));
			break;
		} else {
			usleep(500000); /* half a sec */
		}
	}

	close(serial_fd);

	return 0;
}
