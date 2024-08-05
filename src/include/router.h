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
} Param;

typedef struct {
    Param* params;
    size_t num_params;
} UrlParams;

typedef HttpcRequest* (*middleware_func)(HttpcRequest*, UrlParams*);
typedef HttpcResponse* (*handler_func)(HttpcRequest*, UrlParams*);

typedef struct {
    char* allowed_origins;
    char* allowed_methods;
    char* allowed_headers;
    char* exposed_headers;
    bool allow_credentials;
    int max_age;
} CometCorsConfig;

extern const CometCorsConfig COMET_CORS_DEFAULT_CONFIG;

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
    CometCorsConfig cors_config;
} CometRouter;

CometRouter* router_init(uint16_t port);
int router_add_route(CometRouter* router, const char* route, HttpcMethodType method, handler_func handler);
void router_add_middleware(CometRouter* router, int route_index, middleware_func middleware);

bool router_set_cors_policy(CometRouter* router, CometCorsConfig config);

void router_start(CometRouter* router);
void router_deinit(CometRouter* router);

#endif
