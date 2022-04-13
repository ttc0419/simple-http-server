/**
 * Copyright (c) 2022, William TANG <galaxyking0419@gmail.com>
 */
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <pthread.h>

#include <liburing.h>

#include "common.h"

#define URING_QUEUE_DEPTH 16

#define EVENT_TYPE_READ 0
#define EVENT_TYPE_READ_FILE 1
#define EVENT_TYPE_WRITE 2

struct uring_request {
	unsigned int type;
	int file_fd;
	int socket;
	int vector_count;
	struct iovec vectors[];
};

static size_t last_worker = 0;
static struct io_uring worker_rings[THREAD_POOL_SIZE];

static pthread_t pool[THREAD_POOL_SIZE];
static pthread_mutex_t worker_ring_locks[THREAD_POOL_SIZE] = PTHREAD_MUTEX_INITIALIZER;

static inline void add_read_event(int client_socket)
{
	struct uring_request *request = malloc(
			sizeof(struct uring_request) + sizeof(struct iovec));
	request->type = EVENT_TYPE_READ;
	request->socket = client_socket;
	request->vector_count = 1;
	request->vectors[0].iov_base = malloc(BUFFER_SIZE);
 	request->vectors[0].iov_len = BUFFER_SIZE;

	pthread_mutex_lock(&worker_ring_locks[last_worker]);
	struct io_uring_sqe *sqe = io_uring_get_sqe(&worker_rings[last_worker]);

	io_uring_prep_read(sqe, client_socket, request->vectors[0].iov_base, BUFFER_SIZE, 0);
	io_uring_sqe_set_data(sqe, request);
	io_uring_submit(&worker_rings[last_worker]);

	pthread_mutex_unlock(&worker_ring_locks[last_worker]);
	last_worker = (last_worker + 1) % THREAD_POOL_SIZE;
}

void process_request(size_t thread_id, int client_socket, struct iovec request)
{
	char *path = get_parent_path(request);

	if (access(path, F_OK) == 0) {
		struct stat info;
		stat(path, &info);

		struct uring_request *http_response_request = malloc(
			sizeof(struct uring_request) + sizeof(struct iovec) * 2);
		http_response_request->socket = client_socket;
		http_response_request->type = EVENT_TYPE_READ_FILE;
		http_response_request->vector_count = 2;

		size_t required_size = snprintf(NULL, 0, OK_RESPONSE, info.st_size) + 1;
		char *headers = (char *)malloc(required_size);
		validate_memory(headers);
		sprintf(headers, OK_RESPONSE, info.st_size);

		http_response_request->vectors[0].iov_base = headers;
		http_response_request->vectors[0].iov_len = required_size;

		http_response_request->file_fd = open(path, O_RDONLY);
		http_response_request->vectors[1].iov_base = malloc(info.st_size);
		http_response_request->vectors[1].iov_len = info.st_size;

		pthread_mutex_lock(&worker_ring_locks[thread_id]);
		struct io_uring_sqe *sqe = io_uring_get_sqe(&worker_rings[thread_id]);
		io_uring_prep_read(sqe, http_response_request->file_fd,
			http_response_request->vectors[1].iov_base, info.st_size, 0);
		io_uring_sqe_set_data(sqe, http_response_request);
	} else {
		struct uring_request *error_request = malloc(sizeof(struct uring_request));
		error_request->socket = client_socket;
		error_request->type = EVENT_TYPE_WRITE;
		error_request->vector_count = 0;

		pthread_mutex_lock(&worker_ring_locks[thread_id]);
		struct io_uring_sqe *sqe = io_uring_get_sqe(&worker_rings[thread_id]);
		io_uring_prep_write(sqe, client_socket, ERROR_RESPONSE, sizeof(ERROR_RESPONSE), 0);
		io_uring_sqe_set_data(sqe, error_request);
	}

	io_uring_submit(&worker_rings[thread_id]);
	pthread_mutex_unlock(&worker_ring_locks[thread_id]);
}

_Noreturn
void *handle_connection(void *arg)
{
	signal(SIGPIPE, SIG_IGN);

	size_t thread_id = (size_t)arg;
	struct io_uring_cqe *cqe;

	while (true) {
		io_uring_wait_cqe(&worker_rings[thread_id], &cqe);
		struct uring_request *request = (struct uring_request *)cqe->user_data;

		switch (request->type) {
			case EVENT_TYPE_READ:
				request->vectors[0].iov_len = cqe->res;
				request->vectors[0].iov_base = realloc(request->vectors[0].iov_base, cqe->res);
				process_request(thread_id, request->socket, request->vectors[0]);
				free(request->vectors[0].iov_base);
				free(request);
				break;
			case EVENT_TYPE_READ_FILE:
				pthread_mutex_lock(&worker_ring_locks[thread_id]);
				struct io_uring_sqe *sqe = io_uring_get_sqe(&worker_rings[thread_id]);
				request->type = EVENT_TYPE_WRITE;
				io_uring_prep_writev(sqe, request->socket, request->vectors, request->vector_count, 0);
				io_uring_sqe_set_data(sqe, request);
				io_uring_submit(&worker_rings[thread_id]);
				pthread_mutex_unlock(&worker_ring_locks[thread_id]);
				break;
			case EVENT_TYPE_WRITE:
				if (request->file_fd != 0)
					close(request->file_fd);
				close(request->socket);
				for (size_t i = 0; i < request->vector_count; ++i)
					free(request->vectors[i].iov_base);
				free(request);
				break;
		}

		io_uring_cqe_seen(&worker_rings[thread_id], cqe);
	}
}

int main() {
	int server_socket = setup_tcp_server();

	for (size_t i = 0; i < THREAD_POOL_SIZE; i++) {
		io_uring_queue_init(URING_QUEUE_DEPTH, &worker_rings[i], 0);
		pthread_create(&pool[i], NULL, handle_connection, (void *)i);
	}

	while (true) {
		int client_socket = accept(server_socket, NULL, NULL);
		validate_fd(client_socket);
		add_read_event(client_socket);
	}
}
