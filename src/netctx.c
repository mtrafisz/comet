#include "include/netctx.h"
#include "include/logger.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

typedef SOCKET NetSocket;

#define ERROR_TIMEOUT WSAETIMEDOUT
#define ERROR_CANCELLED WSAEINTR
#define ERROR_WOULD_BLOCK WSAEWOULDBLOCK
#define ERROR_AGAIN WSAEAGAIN
#define GET_ERROR_STR() strerror(WSAGetLastError())
#define CLOSE_SOCKET(s) closesocket(s)
#define SHUTDOWN_SOCKET(s) shutdown(s, SD_BOTH)

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>

typedef int NetSocket;

#define ERROR_TIMEOUT ETIMEDOUT
#define ERROR_CANCELLED EINTR
#define ERROR_WOULD_BLOCK EWOULDBLOCK
#define ERROR_AGAIN EAGAIN
#define GET_ERROR_STR() strerror(errno)
#define CLOSE_SOCKET(s) close(s)
#define SHUTDOWN_SOCKET(s) shutdown(s, SHUT_RDWR)

#endif

#include <stdlib.h>
#include <errno.h>

NetAddress netaddr_from_sockaddr(const struct sockaddr_in *addr) {
    NetAddress ret;
    ret.ip = addr->sin_addr.s_addr;
    ret.port = addr->sin_port;
    return ret;
}

struct sockaddr_in netaddr_to_sockaddr(NetAddress addr) {
    struct sockaddr_in ret;
    ret.sin_family = AF_INET;
    ret.sin_addr.s_addr = addr.ip;
    ret.sin_port = addr.port;
    return ret;
}

NetContext *netctx_init(uint16_t port) {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        log_message(LOG_ERROR, "Failed to initialize Winsock: %s", GET_ERROR_STR());
        return NULL;
    }
#endif

    NetContext *ctx = malloc(sizeof(NetContext));
    if (ctx == NULL) {
        log_message(LOG_ERROR, "Failed to allocate memory for NetContext");
        return NULL;
    }

    ctx->local_addr.ip = INADDR_ANY;
    ctx->local_addr.port = htons(port);
    ctx->local_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->local_sockfd == SOCKET_ERROR) {
        log_message(LOG_ERROR, "Failed to create socket: %s", GET_ERROR_STR());
        free(ctx);
        return NULL;
    }

    struct sockaddr_in local_sockaddr = netaddr_to_sockaddr(ctx->local_addr);
    if (bind(ctx->local_sockfd, (struct sockaddr *)&local_sockaddr, sizeof(local_sockaddr)) == SOCKET_ERROR) {
        log_message(LOG_ERROR, "Failed to bind socket: %s", GET_ERROR_STR());
        free(ctx);
        return NULL;
    }

#ifdef _WIN32
    DWORD timeout = 1000;
#else
    struct timeval timeout = {1, 0};
#endif
    if (setsockopt(ctx->local_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
        log_message(LOG_ERROR, "Failed to set socket receive timeout: %s", GET_ERROR_STR());
        goto error;
    }

    int optval = 1;
    if (setsockopt(ctx->local_sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval)) == SOCKET_ERROR) {
        log_message(LOG_ERROR, "Failed to set socket option SO_REUSEADDR: %s", GET_ERROR_STR());
        goto error;
    }

    if (listen(ctx->local_sockfd, 10) == SOCKET_ERROR) {
        log_message(LOG_ERROR, "Failed to listen on socket: %s", GET_ERROR_STR());
        goto error;
    }

    log_message(LOG_INFO, "Listening on port %d", port);
    return ctx;
error:
    SHUTDOWN_SOCKET(ctx->local_sockfd);
    CLOSE_SOCKET(ctx->local_sockfd);
    free(ctx);
    return NULL;
}

NetSocket netctx_get_next_connection(NetContext *ctx) {
    struct sockaddr_in remote_sockaddr;
    socklen_t remote_sockaddr_len = sizeof(remote_sockaddr);
    NetSocket remote_sockfd = accept(ctx->local_sockfd, (struct sockaddr *)&remote_sockaddr, &remote_sockaddr_len);
    if (remote_sockfd == SOCKET_ERROR) {
        if (errno == ERROR_TIMEOUT || errno == ERROR_CANCELLED || errno == ERROR_WOULD_BLOCK || errno == ERROR_AGAIN) {
            return SOCKET_ERROR;
        }

        log_message(LOG_ERROR, "Failed to accept connection: %s", GET_ERROR_STR());
        return SOCKET_ERROR;
    }

    ctx->remote_addr = netaddr_from_sockaddr(&remote_sockaddr);
    ctx->remote_sockfd = remote_sockfd;
    if (verbose_output) {
        log_message(LOG_INFO, "Accepted connection from %s:%d",
                    inet_ntoa(remote_sockaddr.sin_addr), ntohs(remote_sockaddr.sin_port));
    }
    return remote_sockfd;
}

void netctx_close_current_connection(NetContext *ctx) {
    SHUTDOWN_SOCKET(ctx->remote_sockfd);
    CLOSE_SOCKET(ctx->remote_sockfd);
    if (verbose_output) {
        log_message(LOG_INFO, "Closed connection from %s:%d",
                    inet_ntoa(netaddr_to_sockaddr(ctx->remote_addr).sin_addr),
                    ntohs(netaddr_to_sockaddr(ctx->remote_addr).sin_port));
    }
}

void netctx_deinit(NetContext *ctx) {
    if (!ctx) {
        log_message(LOG_WARN, "Attempted to deinitialize NULL NetContext");
        return;
    }

    if (ctx->local_sockfd != SOCKET_ERROR) {
        SHUTDOWN_SOCKET(ctx->local_sockfd);
        CLOSE_SOCKET(ctx->local_sockfd);
    }
    free(ctx);
#ifdef _WIN32
    WSACleanup();
#endif
}

ByteCount netctx_send(NetContext *ctx, const void *buf, size_t len) {
    ByteCount sent = send(ctx->remote_sockfd, buf, len, 0);
    if (sent == SOCKET_ERROR) {
        log_message(LOG_ERROR, "Failed to send data: %s", GET_ERROR_STR());
    }
    return sent;
}

ByteCount netctx_recv(NetContext *ctx, void *buf, size_t len) {
    ByteCount received = recv(ctx->remote_sockfd, buf, len, 0);
    if (received == SOCKET_ERROR) {
        log_message(LOG_ERROR, "Failed to receive data: %s", GET_ERROR_STR());
    }
    return received;
}

bool netctx_config_timeout(NetSocket sockfd, int send_timeout_ms, int recv_timeout_ms) {
#ifndef _WIN32
    struct timeval recv_timeout = {recv_timeout_ms / 1000, (recv_timeout_ms % 1000) * 1000};
    struct timeval send_timeout = {send_timeout_ms / 1000, (send_timeout_ms % 1000) * 1000};
#else
    DWORD recv_timeout = recv_timeout_ms;
    DWORD send_timeout = send_timeout_ms;
#endif
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&recv_timeout, sizeof(recv_timeout)) == SOCKET_ERROR) {
        log_message(LOG_ERROR, "Failed to set socket receive timeout: %s", GET_ERROR_STR());
        return false;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&send_timeout, sizeof(send_timeout)) == SOCKET_ERROR) {
        log_message(LOG_ERROR, "Failed to set socket send timeout: %s", GET_ERROR_STR());
        return false;
    }

    return true;
}