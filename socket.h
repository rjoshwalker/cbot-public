#ifndef CBOT_NETWORK_H
#define CBOT_NETWORK_H

#include <stdbool.h>
#include <unistd.h>

typedef struct _socket socket_t;

socket_t *_socket(char *ip, char *port);

typedef bool (*socket_connect)(socket_t *this, int seconds);
typedef void (*socket_close)(socket_t *this);
typedef void (*socket_set_rcv_timeo)(socket_t *this, int seconds);
typedef void (*socket_set_snd_timeo)(socket_t *this, int seconds);
typedef int (*socket_read)(socket_t *this, void *buf, size_t n);
typedef int (*socket_write)(socket_t *this, const void *buf, size_t n);

struct _socket {
	void (*free)(void *);

	socket_connect connect;
	socket_close close;

	socket_set_rcv_timeo set_rcv_timeo;
	socket_set_snd_timeo set_snd_timeo;

	socket_read read;
	socket_read read_line;

	socket_write write;
	socket_write write_n;

	int fd;

	char *_ip;
	char *_port;
};

#endif
