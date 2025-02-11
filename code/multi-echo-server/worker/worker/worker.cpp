// worker.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <uv.h>

uv_loop_t *loop;
uv_pipe_t queue;

typedef struct {
	uv_write_t req;
	uv_buf_t buf;
} write_req_t;

void free_write_req(uv_write_t *req) {
	write_req_t *wr = (write_req_t*)req;
	free(wr->buf.base);
	free(wr);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
	buf->base = (char*)malloc(suggested_size);
	buf->len = suggested_size;
}

void echo_write(uv_write_t *req, int status) {
	if (status) {
		fprintf(stderr, "Write error %s\n", uv_err_name(status));
	}
	fprintf(stderr, "Write error %s\n", uv_err_name(status));
	free_write_req(req);
}

void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
	if (nread > 0) {
		char *pReadBuf = new char[nread + 1];
		memset(pReadBuf, 0, nread + 1);
		memcpy(pReadBuf, buf->base, nread);
		fprintf(stderr, "Read data %s\n", pReadBuf);
		delete[]pReadBuf;
		write_req_t *req = (write_req_t*)malloc(sizeof(write_req_t));
		req->buf = uv_buf_init(buf->base, nread);
		uv_write((uv_write_t*)req, client, &req->buf, 1, echo_write);
		return;
	}

	if (nread < 0) {
		if (nread != UV_EOF)
			fprintf(stderr, "Read error %s\n", uv_err_name(nread));
		uv_close((uv_handle_t*)client, NULL);
	}

	free(buf->base);
}

void on_new_connection(uv_stream_t *q, ssize_t nread, const uv_buf_t *buf) {
	if (nread < 0) {
		if (nread != UV_EOF)
			fprintf(stderr, "Read error %s\n", uv_err_name(nread));
		uv_close((uv_handle_t*)q, NULL);
		return;
	}

	uv_pipe_t *pipe = (uv_pipe_t*)q;
	if (!uv_pipe_pending_count(pipe)) {
		fprintf(stderr, "No pending count\n");
		return;
	}

	uv_handle_type pending = uv_pipe_pending_type(pipe);
	assert(pending == UV_TCP);

	uv_tcp_t *client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, client);
	if (uv_accept(q, (uv_stream_t*)client) == 0) {
		uv_os_fd_t fd;
		uv_fileno((const uv_handle_t*)client, &fd);
#ifdef WIN32
		fprintf(stderr, "Worker %d: Accepted fd %d\n", ::GetCurrentProcessId(), fd);
#else
		fprintf(stderr, "Worker %d: Accepted fd %d\n", getpid(), fd);
#endif
		uv_read_start((uv_stream_t*)client, alloc_buffer, echo_read);
	}
	else {
		uv_close((uv_handle_t*)client, NULL);
	}
}

int main() {

	loop = uv_default_loop();

	uv_pipe_init(loop, &queue, 1 /* ipc */);
	int status = uv_pipe_open(&queue, 0);
	if (status) {
		fprintf(stderr, "uv_pipe_open error %s\n", uv_err_name(status));
	}
	status = uv_read_start((uv_stream_t*)&queue, alloc_buffer, on_new_connection);
	if (status) {
		fprintf(stderr, "uv_read_start error %s\n", uv_err_name(status));
	}
	return uv_run(loop, UV_RUN_DEFAULT);
}

