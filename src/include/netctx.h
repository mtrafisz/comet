#ifndef _COMET_NET_CTX_H
#define _COMET_NET_CTX_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t ip;
    uint16_t port;
    uint16_t padding;
} NetAddress;

#ifdef _WIN32
#include <winsock2.h>
#define SOCKET_ERROR INVALID_SOCKET
typedef int ByteCount;
#else
#define SOCKET_ERROR -1
#include <sys/types.h>
#include <netinet/in.h>
typedef int NetSocket;
typedef ssize_t ByteCount;
#endif

NetAddress netaddr_from_sockaddr(const struct sockaddr_in *addr);
struct sockaddr_in netaddr_to_sockaddr(NetAddress addr);

typedef struct {
    NetAddress local_addr;
    NetAddress remote_addr;
    NetSocket local_sockfd;
    NetSocket remote_sockfd;
} NetContext;

bool netctx_init(NetContext **out_ctx, uint16_t port);
NetSocket netctx_get_next_connection(NetContext *ctx);
void netctx_close_current_connection(NetContext *ctx);
void netctx_deinit(NetContext *ctx);

bool netctx_config_timeout(NetSocket sockfd, int send_timeout_ms, int recv_timeout_ms);

ByteCount netctx_send(NetContext *ctx, const void *buf, size_t len);
ByteCount netctx_recv(NetContext *ctx, void *buf, size_t len);

#endif
