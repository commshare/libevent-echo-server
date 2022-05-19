// echo-server.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
//https://github.com/eddieh/libevent-echo-server/blob/master/echo-server.c
/*
 * echo-server.c
 *
 * This is a modified version of the "simpler ROT13 server with
 * Libevent" from:
 * http://www.wangafu.net/~nickm/libevent-book/01_intro.html
 */

//这4个linux才有
//#include <netinet/in.h>
//#include <sys/socket.h>
//#include <unistd.h>
//#include <fcntl.h>


#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
//#include <getopt.h>
#include <io.h>
#include <fcntl.h>
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif
#else /* !_WIN32 */
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#endif /* _WIN32 */
#include <signal.h>



#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <assert.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define MAX_LINE 16384


#ifdef _WIN32
#ifndef stat
#define stat _stat
#endif
#ifndef fstat
#define fstat _fstat
#endif
#ifndef open
#define open _open
#endif
#ifndef close
#define close _close
#endif
#ifndef O_RDONLY
#define O_RDONLY _O_RDONLY
#endif
#endif /* _WIN32 */



void do_accept(evutil_socket_t listener, short event, void* arg);
void read_cb(struct bufferevent* bev, void* arg);
void error_cb(struct bufferevent* bev, short event, void* arg);
void write_cb(struct bufferevent* bev, void* arg);




void
on_read(struct bufferevent* bev, void* ctx)
{
    struct evbuffer* input, * output;
    char* line;
    size_t n;
   // int i;

    input = bufferevent_get_input(bev);
    output = bufferevent_get_output(bev);

    while ((line = evbuffer_readln(input, &n, EVBUFFER_EOL_LF))) {
        evbuffer_add(output, line, n);
        evbuffer_add(output, "\n", 1);
        free(line);
    }

    if (evbuffer_get_length(input) >= MAX_LINE) {
        /* line is too long */
        char buf[1024];
        while (evbuffer_get_length(input)) {
            int n = evbuffer_remove(input, buf, sizeof(buf));
            evbuffer_add(output, buf, n);
        }
        evbuffer_add(output, "\n", 1);
    }
}

void
on_error(struct bufferevent* bev, short error, void* ctx)
{
    if (error & BEV_EVENT_EOF) {
    }
    else if (error & BEV_EVENT_ERROR) {
    }
    else if (error & BEV_EVENT_TIMEOUT) {
    }
    bufferevent_free(bev);
}

void
on_accept(evutil_socket_t listener, short event, void* arg)
{
    struct event_base* base = (struct event_base*)arg;
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int fd = accept(listener, (struct sockaddr*)&ss, &slen);
    if (fd < 0) {
        perror("accept");
    }
  //  else if (fd > FD_SETSIZE) {
  //      //https://blog.csdn.net/zhangxiao93/article/details/70159767
		////    if (fd > FD_SETSIZE) { //这个if是参考了那个ROT13的例子，貌似是官方的疏漏，从select-based例子里抄过来忘了改
  //    //  close(fd);
  //  }
    else {
		printf("ACCEPT: fd = %u\n", fd);

        struct bufferevent* bev;
        evutil_make_socket_nonblocking(fd);
        bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        bufferevent_setcb(bev, on_read, NULL, on_error, NULL);
        bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
        bufferevent_enable(bev, EV_READ | EV_WRITE);
    }
}

int
main(int argc, char** argv)
{
    event_enable_debug_logging(EVENT_DBG_ALL);
#ifdef _WIN32
	{
		WORD wVersionRequested;
		WSADATA wsaData;
		wVersionRequested = MAKEWORD(2, 2);
		WSAStartup(wVersionRequested, &wsaData);
	}
#else
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		ret = 1;
		goto err;
	}
#endif

    setvbuf(stdout, NULL, _IONBF, 0);

    evutil_socket_t listener;
    struct sockaddr_in sin;
    struct event_base* base;
    struct event* listener_event;

    base = event_base_new();
    if (!base)
        return 1;

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(40713);


    listener = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(listener);

    /* win32 junk? */

    if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(listener, 16) < 0) {
        perror("listen");
        return 1;
    }

    listener_event = event_new(base, listener, EV_READ | EV_PERSIST, on_accept, (void*)base);
    /* check it? */
    event_add(listener_event, NULL);

    event_base_dispatch(base);


#ifdef _WIN32
	WSACleanup();
#endif
err:
	//if (cfg)
	//	event_config_free(cfg);
	//if (http)
	//	evhttp_free(http);
	//if (term)
	//	event_free(term);
	if (base)
		event_base_free(base);
    return 0;
}

/*
 * echo-server.c
 */
#if  0
https://github.com/eddieh/libuv-echo-server/blob/master/echo-server.c



#include <uv.h>
#include <stdio.h>
#include <stdlib.h>

uv_tcp_t server;
uv_loop_t* loop;

void
alloc_buffer(uv_handle_t* handle, size_t size, uv_buf_t* buf)
{
    buf->base = malloc(size);
    buf->len = size;
}

void
on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));

    if (nread == -1) {
        /* if (uv_last_error(loop).code != UV_EOF) { */
        /* } */

        uv_close((uv_handle_t*)stream, NULL);
    }

    int r = uv_write(req, stream, buf, 1, NULL);

    if (r) {
        /* error */
    }

    free(buf->base);
}

void
on_connection(uv_stream_t* server, int status)
{
    uv_tcp_t* client = malloc(sizeof(uv_tcp_t));

    if (status == -1) {
        /* error */
    }

    uv_tcp_init(loop, client);

    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        int r = uv_read_start((uv_stream_t*)client, alloc_buffer, on_read);

        if (r) {
            /* error */
        }
    }
    else {
        uv_close((uv_handle_t*)client, NULL);
    }
}

int
main(int argc, char** argv)
{
    loop = uv_default_loop();

    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", 3000, &addr);

    uv_tcp_init(loop, &server);
    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);

    int r = uv_listen((uv_stream_t*)&server, 128, on_connection);

    if (r) {
        /* error */
    }

    return uv_run(loop, UV_RUN_DEFAULT);
}


#endif