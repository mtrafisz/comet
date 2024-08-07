#ifndef _COMET_ROUTER_H
#define _COMET_ROUTER_H

#include "netctx.h"
#include <httpc.h>

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @brief A struct to hold a key-value pair.
 */
typedef struct {
    char* key;
    char* value;
} Param;

/**
 * @brief A struct to hold a list of key-value pairs.
 */
typedef struct {
    Param* params;
    size_t num_params;
} UrlParams;

typedef HttpcRequest* (*middleware_func)(void*, HttpcRequest*, UrlParams*);
typedef HttpcResponse* (*handler_func)(void*, HttpcRequest*, UrlParams*);

/**
 * @brief A struct to hold the configuration for CORS.
 */
typedef struct {
    char* allowed_origins;
    char* allowed_methods;
    char* allowed_headers;
    char* exposed_headers;
    bool allow_credentials;
    int max_age;
} CometCorsConfig;

extern const CometCorsConfig COMET_CORS_DEFAULT_CONFIG;

/**
 * @brief A struct to hold a route.
 */
typedef struct {
    char* route;
    HttpcMethodType method;
    handler_func handler;
    middleware_func* middleware_chain;
    size_t num_middleware;
} CometRoute;

/**
 * @brief A struct containing the router's state.
 */
typedef struct {
    NetContext* ctx;
    CometRoute* routes;
    size_t num_routes;
    volatile bool running;
    CometCorsConfig cors_config;
    void* state;
} CometRouter;

/**
 * @brief Initialize a new CometRouter.
 * 
 * Resulting router will only stop on error, or when router->running is set to false;
 * This can be done via a signal handler, for example.
 * 
 * After main loop is done, router_deinit is called to clean up - You should not call it manually.
 * 
 * @param port The port to listen on.
 * @param state A pointer to the state to pass to the handlers and middlewares.
 * @return A pointer to the new CometRouter.
 */
CometRouter* router_init(uint16_t port, void* state);

/**
 * @brief Add a new route to the router.
 * 
 * route parameter can contain:
 * - static parts, like "/hello/world"
 * - dynamic parts, like "/hello/{name}" - `name` will then be available in UrlParams* parameter of the handler
 * - wildcard parts, like "/hello/*" - `*` will then be available in UrlParams* parameter of the handler under "wildcard" key.
 * 
 * @param router The router to add the route to.
 * @param route The route to add.
 * @param method The method to listen for.
 * @param handler The handler to call when the route is hit.
 * @return 0 on success, -1 on error.
 * @warning wildcard part can't be followed by any other part - it consumes the rest of the requested route.
 */
int router_add_route(CometRouter* router, const char* route, HttpcMethodType method, handler_func handler);

/**
 * @brief Add a middleware to a route.
 * 
 * Middleware is called right before executing the handler. Url params are already parsed at this point.
 * 
 * @param router The router to add the middleware to.
 * @param route_index The index of the route to add the middleware to.
 * @param middleware The middleware to add.
 */
void router_add_middleware(CometRouter* router, int route_index, middleware_func middleware);

/**
 * @brief Set the CORS policy for the router.
 * 
 * @param router The router to set the CORS policy for.
 * @param config The CORS policy to set.
 * @return true on success, false on error.
 * @warning It's users responsibility to make sure every field of config is initialized.
 */
bool router_set_cors_policy(CometRouter* router, CometCorsConfig config);

/**
 * @brief Start the router.
 * 
 * This will divert the main thread into the router's main loop.
 * 
 * @param router The router to start.
 */
void router_start(CometRouter* router);
// void router_deinit(CometRouter* router);

#endif
