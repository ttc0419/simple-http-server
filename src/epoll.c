/**
 * Copyright (c) 2022, William TANG <galaxyking0419@gmail.com>
 */
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <pthread.h>

#include "common.h"
#include "queue.h"

#define MAX_EVENT 65536

static int epoll_fd;
static queue_t fd_queue;
static pthread_t pool[THREAD_POOL_SIZE];
static pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_cv = PTHREAD_COND_INITIALIZER;

static inline void add_read_event(int client_socket)
{
	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = client_socket;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event);
}

void process_request(int client_socket, struct iovec request)
{
	char *path = get_parent_path(request);

	if (access(path, F_OK) == 0) {
		struct stat info;
		stat(path, &info);

		size_t required_size = snprintf(NULL, 0, OK_RESPONSE, info.st_size) + 1;
		char *headers = (char *)malloc(required_size);
		validate_memory(headers);
		sprintf(headers, OK_RESPONSE, info.st_size);

		struct iovec response[2];
		response[0].iov_base = headers;
		response[0].iov_len = required_size;

		int file_fd = open(path, O_RDONLY);
		char *file_data = malloc(info.st_size);
		read(file_fd, file_data, info.st_size);

		response[1].iov_base = file_data;
		response[1].iov_len = info.st_size;

		writev(client_socket, response, 2);

		free(headers);
		free(file_data);
		close(file_fd);
	} else {
		write(client_socket, ERROR_RESPONSE, strlen(ERROR_RESPONSE));
	}
}

_Noreturn
void *handle_connection(void *arg)
{
	signal(SIGPIPE, SIG_IGN);

	while (true) {
		pthread_mutex_lock(&queue_lock);
		while (queue_len(&fd_queue) == 0)
			pthread_cond_wait(&queue_cv, &queue_lock);
		int client_socket = *((int *)dequeue(&fd_queue));
		pthread_mutex_unlock(&queue_lock);

		char buffer[BUFFER_SIZE];
		ssize_t bytes_read = read(client_socket, buffer, BUFFER_SIZE);

		struct iovec request;
		request.iov_base = buffer;
		request.iov_len = bytes_read;

		process_request(client_socket, request);

		close(client_socket);
	}
}

int main() {
	int server_socket = setup_tcp_server();

	queue_init(&fd_queue, sizeof(int), MAX_EVENT, 256, 256);

	for (size_t i = 0; i < THREAD_POOL_SIZE; i++)
		pthread_create(&pool[i], NULL, handle_connection, NULL);

	validate_fd(epoll_fd = epoll_create1(0));
	add_read_event(server_socket);
	struct epoll_event events[MAX_EVENT];

	while (true) {
		int count = epoll_wait(epoll_fd, events, MAX_EVENT, -1);
		for (int i = 0; i < count; ++i) {
			if (events[i].data.fd == server_socket) {
				int client_socket = accept(server_socket, NULL, NULL);
				validate_fd(client_socket);
				add_read_event(client_socket);
			} else {
				pthread_mutex_lock(&queue_lock);
				enqueue(&fd_queue, &events[i].data.fd);
				pthread_mutex_unlock(&queue_lock);
				pthread_cond_signal(&queue_cv);
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
			}
		}
	}
}
