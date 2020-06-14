#if !defined(__tunnel_h__)
#define __tunnel_h__ 1

#include <uv.h>
#include <stdbool.h>
#include "sockaddr_universal.h"

struct tunnel_ctx;

enum socket_state {
    socket_state_stop,  /* Stopped. */
    socket_state_busy,  /* Busy; waiting for incoming data or for a write to complete. */
    socket_state_done,  /* Done; read incoming data or write finished. */
    socket_state_dead,
};

struct socket_ctx {
    enum socket_state rdstate;
    enum socket_state wrstate;
    unsigned int idle_timeout;
    struct tunnel_ctx *tunnel;  /* Backlink to owning tunnel context. */
    ssize_t result;
    union uv_any_handle handle;
    bool check_timeout;
    uv_timer_t timer_handle;  /* For detecting timeouts. */
                              /* We only need one of these at a time so make them share memory. */
    union uv_any_req req;
    union sockaddr_universal addr;
    const uv_buf_t *buf; /* Scratch space. Used to read data into. */
};

struct tunnel_ctx {
    void *data;
    bool terminated;
    bool getaddrinfo_pending;
    uv_loop_t *loop; /* Backlink to owning loop object. */
    struct socket_ctx *incoming;  /* Connection with the SOCKS client. */
    struct socket_ctx *outgoing;  /* Connection with upstream. */
    struct socks5_address *desired_addr;
    char extra_info[0x100];
    int ref_count;

    void(*tunnel_dying)(struct tunnel_ctx *tunnel);

    void (*tunnel_dispatcher)(struct tunnel_ctx* tunnel, struct socket_ctx* socket);
    void(*tunnel_timeout_expire_done)(struct tunnel_ctx *tunnel, struct socket_ctx *socket);
    void(*tunnel_outgoing_connected_done)(struct tunnel_ctx *tunnel, struct socket_ctx *socket);
    void(*tunnel_read_done)(struct tunnel_ctx *tunnel, struct socket_ctx *socket);
    void(*tunnel_arrive_end_of_file)(struct tunnel_ctx *tunnel, struct socket_ctx *socket);
    void(*tunnel_getaddrinfo_done)(struct tunnel_ctx *tunnel, struct socket_ctx *socket);
    void(*tunnel_write_done)(struct tunnel_ctx *tunnel, struct socket_ctx *socket);
    size_t(*tunnel_get_alloc_size)(struct tunnel_ctx *tunnel, struct socket_ctx *socket, size_t suggested_size);
    uint8_t*(*tunnel_extract_data)(struct socket_ctx *socket, void*(*allocator)(size_t size), size_t *size);
    bool(*tunnel_is_in_streaming)(struct tunnel_ctx* tunnel);
    void (*tunnel_shutdown)(struct tunnel_ctx *tunnel);
};

uv_os_sock_t uv_stream_fd(const uv_tcp_t *handle);
uint16_t get_socket_port(const uv_tcp_t *tcp);
size_t _update_tcp_mss(struct socket_ctx *socket);
size_t get_fd_tcp_mss(uv_os_sock_t fd);
size_t socket_arrived_data_size(struct socket_ctx *socket, size_t suggested_size);

typedef bool(*tunnel_init_done_cb)(struct tunnel_ctx *tunnel, void *p);
struct tunnel_ctx * tunnel_initialize(uv_loop_t *loop, uv_tcp_t *listener, unsigned int idle_timeout, tunnel_init_done_cb init_done_cb, void *p);

void tunnel_add_ref(struct tunnel_ctx *tunnel);
void tunnel_release(struct tunnel_ctx *tunnel);
bool tunnel_is_dead(struct tunnel_ctx *tunnel);
int socket_connect(struct socket_ctx *socket);
bool socket_is_readable(struct socket_ctx *socket);
bool socket_is_writeable(struct socket_ctx *socket);
void socket_read(struct socket_ctx *socket, bool check_timeout);
void socket_getaddrinfo(struct socket_ctx *socket, const char *hostname);
void socket_write(struct socket_ctx *socket, const void *data, size_t len);
void socket_dump_error_info(const char *title, struct socket_ctx *socket);

#endif // !defined(__tunnel_h__)
