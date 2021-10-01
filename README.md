## chronosync

A small utility to set the clock on a
[Hayes Stack Chronograph](http://atari8bitads.blogspot.com/2017/06/theres-no-better-time.html)
over its serial port.

### Synopsis

	chronosync [-d] [-s serial speed] <serial device>

### Compiling

Compile with a BSD Make:

	$ make

Install:

	# make install

Run and pass argument of the serial device connected to the Chronograph:

	# chronosync /dev/cua02

The computer's local time is sent to the Chronograph.

### Notes

Since the Chronograph only supports setting the time to minute precision with
`ATST`, `chronosync` will sleep until zero seconds of the next minute before
sending the time.

The clock on the Chronograph will probably only need to be set once per day to
stay accurate, so running this from cron once a day should suffice:

	# sync time at 2am
	1	59	*	*	*	/usr/local/bin/chronosync /dev/cua02
