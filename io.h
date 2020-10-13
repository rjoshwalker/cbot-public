#ifndef CBOT_IO_H
#define CBOT_IO_H

#include <unistd.h>
#include <stddef.h>
#include <stdio.h>

typedef struct __io io_t;

struct __io {
	int (*read_line)(int fd, char *buf, size_t size);
	int (*read_until)(int fd, char *buf, size_t size, char delimiter);
	ssize_t (*read_n)(int fd, char *buf, size_t n);
	ssize_t (*write_n)(int fd, const char *buf, size_t n);
};

const io_t __io;
const io_t *_io;

#endif
