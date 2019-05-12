#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#define SERVER_PORT 8888
/// backlog queue – the maximum length of queued connections
/// for tcp connection
#define DEFAULT_BACKLOG 10000

uv_loop_t *loop;
uv_udp_t recv_socket;

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    (void)handle;
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

/// called after the data was sent
static void on_send(uv_udp_send_t* req, int status)
{
    free(req);
    if (status) {
        fprintf(stderr, "uv_udp_send_cb error: %s\n", uv_strerror(status));
    }
}

void on_read(uv_udp_t *req, ssize_t nread, const uv_buf_t *buf,
    const struct sockaddr *addr, unsigned flags)
{
    (void)flags;
    if (nread < 0) {
        fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t*) req, NULL);
        free(buf->base);
        return;
    }

    if (nread > 0) {
        char sender[17] = { 0 };
        uv_ip4_name((const struct sockaddr_in*) addr, sender, 16);
        fprintf(stderr, "Recv from %s\n", sender);

        uv_udp_send_t* res = malloc(sizeof(uv_udp_send_t));
        uv_buf_t buff = uv_buf_init(buf->base, nread);
        uv_udp_send(res, req, &buff, 1, addr, on_send);
    }

    free(buf->base);
}

static void on_walk_cleanup(uv_handle_t* handle, void* data)
{
    (void)data;
    uv_close(handle, NULL);
}

static void on_server_exit()
{
    printf("on_server_exit\n");
    // clean all stuff
    uv_stop(uv_default_loop());
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_walk(uv_default_loop(), on_walk_cleanup, NULL);
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());
    exit(0);
}

static void on_signal(uv_signal_t* signal, int signum)
{
    (void)signum;
    printf("on_signal\n");
    if(uv_is_active((uv_handle_t*)&recv_socket)) {
        uv_udp_recv_stop(&recv_socket);
    }
    uv_close((uv_handle_t*) &recv_socket, on_server_exit);
    uv_signal_stop(signal);
}

static inline void init_udp_server(uv_loop_t *loop, const char* address, uint16_t port)
{
    struct sockaddr_in recv_addr;
    uv_ip4_addr(address, port, &recv_addr);

    uv_udp_init(loop, &recv_socket);

    uv_udp_bind(&recv_socket, (const struct sockaddr *)&recv_addr, UV_UDP_REUSEADDR);
    uv_udp_recv_start(&recv_socket, alloc_buffer, on_read);
}

static inline void init_signal(uv_signal_t* signal, int signum)
{
    uv_signal_init(loop, signal);
    uv_signal_start(signal, on_signal, signum);
}

int main(int argc, const char** argv)
{
    (void)argc; (void) argv;
    loop = uv_default_loop();

    uv_signal_t sigkill, sigterm, sigint;
    init_signal(&sigkill, SIGKILL);

    init_signal(&sigterm, SIGTERM);
    init_signal(&sigint, SIGINT);

    init_udp_server(loop, "0.0.0.0", SERVER_PORT);

    printf("Server listening on: %d\n", SERVER_PORT);
    fflush(stdout);
    return uv_run(loop, UV_RUN_DEFAULT);
}