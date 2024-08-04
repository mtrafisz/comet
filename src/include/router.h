#ifndef _COMET_ROUTER_H
#define _COMET_ROUTER_H

#include "netctx.h"
#include <httpc.h>

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    char* key;
    char* value;
} UrlParam;

typedef struct {
    UrlParam* params;
    size_t num_params;
} UrlParams;

typedef HttpcRequest* (*middleware_func)(HttpcRequest*, UrlParams*);
typedef HttpcResponse* (*handler_func)(HttpcRequest*, UrlParams*);

typedef struct {
    char* route;
    HttpcMethodType method;
    handler_func handler;
    middleware_func* middleware_chain;
    size_t num_middleware;
} CometRoute;

typedef struct {
    NetContext* ctx;
    CometRoute* routes;
    size_t num_routes;
    volatile bool running;
} CometRouter;

CometRouter* router_init(uint16_t port);
int router_add_route(CometRouter* router, const char* route, HttpcMethodType method, handler_func handler);
void router_add_middleware(CometRouter* router, int route_index, middleware_func middleware);

void router_start(CometRouter* router);
void router_deinit(CometRouter* router);

#endif
