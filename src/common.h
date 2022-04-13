/**
 * Copyright (c) 2022, William TANG <galaxyking0419@gmail.com>
 */
#ifndef SIMPLE_HTTP_SERVER_COMMON_H
#define SIMPLE_HTTP_SERVER_COMMON_H

#include <errno.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#define ADDRESS "0.0.0.0"
#define PORT 8080

#define BUFFER_SIZE 4096
#define THREAD_POOL_SIZE 2

#define ERROR_RESPONSE "HTTP/1.1 404 Not Found\r\nConnection: close\r\n"
#define OK_RESPONSE "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/javascript\r\nContent-Length: %ld\r\n\r\n"

static inline void validate_fd(int fd)
{
	if (fd < 0) {
		fprintf(stderr, "[FATAL] %s, code %d\n", strerror(errno), errno);
		exit(-1);
	}
}

static inline void validate_memory(void *ptr)
{
	if (ptr == NULL) {
		fprintf(stderr, "[FATAL] Cannot allocate enough memory\n");
		exit(-1);
	}
}

static inline int setup_tcp_server()
{
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(PORT);
	address.sin_addr.s_addr = inet_addr(ADDRESS);

	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	validate_fd(server_socket);

	validate_fd(bind(server_socket, (const struct sockaddr *)&address, sizeof(address)));

	validate_fd(listen(server_socket, 65535));
	printf("[INFO] Listening on %s:%hu...\n", ADDRESS, PORT);

	return server_socket;
}

static inline char *get_parent_path(struct iovec request)
{
	char *base = (char *)request.iov_base;
	if (request.iov_len < 18 || base[request.iov_len - 4] != '\r' || base[request.iov_len - 3] != '\n'
			|| base[request.iov_len - 2] != '\r' || base[request.iov_len - 1] != '\n')
		return NULL;

	strtok(request.iov_base, " ");
	char *path = basename(strtok(NULL, " "));

	*(path - 3) = '.';
	*(path - 2) = '.';
	*(path - 1) = '/';
	path -= 3;

	return path;
}
#endif
