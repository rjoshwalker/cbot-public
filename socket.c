#include "socket.h"
#include "io.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/select.h>
#include <stdio.h>
#include <errno.h>

static void _free(void *obj);
static bool _connect(socket_t *this, int seconds);
static void _fd_close(socket_t *this);
static void _set_rcv_timeo(socket_t *this, int seconds);
static void _set_snd_timeo(socket_t *this, int seconds);
static int _read(socket_t *this, void *buf, size_t n);
static int _write(socket_t *this, const void *buf, size_t n);
static int _write_n(socket_t *this, const void *buf, size_t n);

static void _free(void *obj)
{
	socket_t *this = (socket_t *)obj;
	_fd_close(this);
	free(this);
}

static bool _connect(socket_t *this, int seconds)
{
	struct addrinfo hints, *result, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int s = getaddrinfo(this->_ip, this->_port, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		return false;
	}

	for (p = result; p != NULL; p = p->ai_next) {
		int fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

		/* Keep trying to make a socket */
		if (fd == -1)
			continue;

		/* Turn on non-blocking */
		{
			int flags = fcntl(fd, F_GETFL, 0);
			if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
				close(fd);
				continue;
			}
		}

		/* Start connect */
		{
			int c;
			if ((c = connect(fd, p->ai_addr, p->ai_addrlen)) < 0) {
				/* Continue anyways, coverity complains otherwise */
			}
		}

		/* Setup select set */
		fd_set write;
		FD_ZERO(&write);
		FD_SET(fd, &write);

		/* Setup max wait timeout */
		struct timeval tv = { .tv_sec = seconds, .tv_usec = 0 };

		if (select(fd+1, NULL, &write, NULL, &tv) == 1) {
			int err;
			socklen_t len = sizeof(err);

			getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);

			/* Turn off non-blocking */
			{
				int flags = fcntl(fd, F_GETFL, 0);
				fcntl(fd, F_SETFL, flags & (~O_NONBLOCK));
			}

			/* No error, keep socket */
			if (err == 0)
				this->fd = fd;
			else
				close(fd);
		} else {
			/* Timeout has been reached */
			close(fd);
		}

		break;
	}

	freeaddrinfo(result);

	return (this->fd > 0);
}

static void _fd_close(socket_t *this)
{
	if (this->fd > 0)
		close(this->fd);
}

static void _set_rcv_timeo(socket_t *this, int seconds)
{
	if (this->fd == 0)
		return;

	struct timeval tv = { .tv_sec = seconds, .tv_usec = 0 };
	setsockopt(this->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

static void _set_snd_timeo(socket_t *this, int seconds)
{
	if (this->fd == 0)
		return;

	struct timeval tv = { .tv_sec = seconds, .tv_usec = 0 };
	setsockopt(this->fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

static int _read(socket_t *this, void *buf, size_t n)
{
	return (int) read(this->fd, buf, n);
}

static int _read_line(socket_t *this, void *buf, size_t n)
{
	return _io->read_line(this->fd, buf, n);
}

static int _write(socket_t *this, const void *buf, size_t n)
{
	return (int) write(this->fd, buf, n);
}

static int _write_n(socket_t *this, const void *buf, size_t n)
{
	return (int) _io->write_n(this->fd, buf, n);
}

socket_t *_socket(char *ip, char *port)
{
	socket_t *new = calloc(1, sizeof(*new));
	if (!new) return NULL;

	new->free = _free;

	new->connect = _connect;
	new->close = _fd_close;

	new->set_rcv_timeo = _set_rcv_timeo;
	new->set_snd_timeo = _set_snd_timeo;

	new->read = _read;
	new->read_line = _read_line;
	new->write = _write;
	new->write_n = _write_n;

	new->_ip = ip;
	new->_port = port;

	return new;
}
