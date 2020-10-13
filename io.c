#include "io.h"
#include <unistd.h>
#include <errno.h>

int _read_line(int fd, char *buf, size_t size);
int _read_until(int fd, char *buf, size_t size, char delimiter);
ssize_t _read_n(int fd, char *buf, size_t n);
ssize_t _write_n(int fd, const char *buf, size_t n);

const io_t _cob_io = {
		.read_line = _read_line,
		.read_until = _read_until,
		.read_n = _read_n,
		.write_n = _write_n
};

const io_t *_io = &_cob_io;

int _read_line(int fd, char *buf, size_t size)
{
	char c, *bp = buf;

	while (1) {
		size_t s = read(fd, &c, 1);

		if (s == -1)
			return -1;

		/* Return EOF or what we've captured so far */
		if (s == 0) {
			if (bp == buf)
				return 0;
			else
				break;
		}

		/* Space left in buffer? */
		if ((bp - buf) / sizeof(*buf) < size)
			*bp++ = c;
		else
			return -2;

		if (c == '\n')
			break;
	}

	*bp = '\0';
	return ((bp - buf) / sizeof(*buf));
}

int _read_until(int fd, char *buf, size_t size, char delimiter)
{
	char c, *bp = buf;

	while (1) {
		ssize_t s = read(fd, &c, 1);

		/* Return EOF or what we've captured so far */
		if (s < 1) {
			if (bp == buf)
				return 0;
			else
				break;
		}

		/* Space left in buffer? */
		if ((bp - buf) / sizeof(*buf) < size)
			*bp++ = c;
		else
			return -2;

		if (c == delimiter)
			break;
	}

	*bp = '\0';
	return (int) ((bp - buf) / sizeof(*buf));
}

ssize_t _read_n(int fd, char *buf, size_t n)
{
	size_t total = 0;

	while (total != n) {
		int bytes = read(fd, buf+total, n-total);

		if (bytes == -1) {
			if (errno == EINTR)
				continue;
			else
				return -1;
		}

		if (bytes == 0)
			return total;

		total += bytes;
	}

	return total;
}

ssize_t _write_n(int fd, const char *buf, size_t n)
{
	size_t total = 0;

	while (total != n) {
		int bytes = write(fd, buf+total, n-total);

		if (bytes == -1) {
			if (errno == EINTR)
				continue;
			else
				return -1;
		}

		total += bytes;
	}

	return total;
}
