#include "include/router.h"
#include "include/netctx.h"
#include "include/logger.h"

#include <signal.h>

HttpcResponse* default_not_found_handler(HttpcRequest* req) {
    HttpcResponse* res = httpc_response_new("Not Found", 404);
    httpc_response_set_body(res, "404 Not Found", 13);
    httpc_add_header_v(&res->headers, "Content-Type", "text/plain");
    return res;
}

CometRouter* router_init(uint16_t port) {
    CometRouter* router = malloc(sizeof(CometRouter));
    if (router == NULL) {
        log_message(LOG_ERROR, "Failed to allocate memory for router");
        return NULL;
    }

    router->ctx = netctx_init(port);
    if (router->ctx == NULL) {
        free(router);
        return NULL;
    }

    router->num_routes = 0;
    router->routes = NULL;
    router->running = false;
    
    log_message(LOG_INFO, "Router has been initialized");

    return router;
}

int router_add_route(CometRouter* router, const char* route, HttpcMethodType method, handler_func handler) {
    CometRoute* new_routes = realloc(router->routes, (router->num_routes + 1) * sizeof(CometRoute));
    if (new_routes == NULL) {
        log_message(LOG_ERROR, "Failed to allocate memory for new route");
        return -1;
    }

    router->routes = new_routes;
    router->routes[router->num_routes].route = strdup(route);
    router->routes[router->num_routes].method = method;
    router->routes[router->num_routes].handler = handler;
    router->routes[router->num_routes].middleware_chain = NULL;
    router->routes[router->num_routes].num_middleware = 0;

    router->num_routes++;

    return router->num_routes - 1;
}

void router_add_middleware(CometRouter* router, int route_index, middleware_func middleware) {
    if (route_index < 0 || !router || (route_index >= router->num_routes && route_index != INT32_MAX)) {
        log_message(LOG_ERROR, "Invalid route index");
        return;
    }

    CometRoute* route = &router->routes[route_index];
    middleware_func* new_chain = realloc(route->middleware_chain, (route->num_middleware + 1) * sizeof(middleware_func));
    if (new_chain == NULL) {
        log_message(LOG_ERROR, "Failed to allocate memory for new middleware chain");
        return;
    }

    route->middleware_chain = new_chain;
    route->middleware_chain[route->num_middleware] = middleware;
    route->num_middleware++;
}

HttpcRequest* router_read_next_request(CometRouter* router) {
    if (!router) {
        log_message(LOG_ERROR, "Router is NULL");
        return NULL;
    }

    NetSocket client_sock = netctx_get_next_connection(router->ctx);
    if (client_sock == SOCKET_ERROR) {
        return NULL;
    }

    char* request = malloc(1024);
    size_t request_len = 0;
    if (request == NULL) {
        log_message(LOG_ERROR, "Failed to allocate memory for request");
        return NULL;
    }
    char buffer[1024];
    ByteCount bytes_read = 0;

    do {
        bytes_read = netctx_recv(router->ctx, buffer, sizeof(buffer));
        if (bytes_read == SOCKET_ERROR) {
            log_message(LOG_ERROR, "Failed to read from socket");
            free(request);
            return NULL;
        }

        request = realloc(request, request_len + bytes_read);
        if (request == NULL) {
            log_message(LOG_ERROR, "Failed to reallocate memory for request");
            return NULL;
        }

        memcpy(request + request_len, buffer, bytes_read);
        request_len += bytes_read;
    } while (bytes_read == sizeof(buffer));

    HttpcRequest* req = httpc_request_from_string(request, request_len);
    if (req == NULL) {
        log_message(LOG_ERROR, "Failed to parse request");
        free(request);
        return NULL;
    }

    free(request);

    return req;
}

char** split_string_by_delim(const char* str, const char* delim, size_t* num_tokens) {
    char* str_copy = strdup(str);
    if (str_copy == NULL) {
        return NULL;
    }

    char** tokens = malloc(1 * sizeof(char*));

    size_t count = 0;
    char* token = strtok(str_copy, delim);

    while (token != NULL) {
        tokens = realloc(tokens, (count + 1) * sizeof(char*));
        if (tokens == NULL) {
            free(str_copy);
            return NULL;
        }

        tokens[count] = strdup(token);
        if (tokens[count] == NULL) {
            for (size_t i = 0; i < count; i++) {
                free(tokens[i]);
            }
            free(tokens);
            free(str_copy);
            return NULL;
        }

        count++;
        token = strtok(NULL, delim);
    }

    free(str_copy);
    *num_tokens = count;
    return tokens;
}

bool extract_url_params(const char *route_pattern_s, const char *actual_url_s, UrlParams *params) {
    bool result = true;
    
    size_t num_route_tokens;
    char** route_tokens = split_string_by_delim(route_pattern_s, "/", &num_route_tokens);
    if (route_tokens == NULL) {
        return false;
    }

    size_t num_url_tokens;
    char** url_tokens = split_string_by_delim(actual_url_s, "/", &num_url_tokens);
    if (url_tokens == NULL) {
        free(route_tokens);
        return false;
    }

    if (num_route_tokens != num_url_tokens) {
        result = false;
        goto cleanup;
    }

    for (size_t i = 0; i < num_route_tokens; i++) {
        if (route_tokens[i][0] == '{' && route_tokens[i][strlen(route_tokens[i]) - 1] == '}') {
            char *param_name = strndup(route_tokens[i] + 1, strlen(route_tokens[i]) - 2);
            char *param_value = strdup(url_tokens[i]);

            UrlParam *new_params = realloc(params->params, (params->num_params + 1) * sizeof(UrlParam));
            if (new_params == NULL) {
                free(param_name);
                free(param_value);
                
                result = false;
                goto cleanup;
            }
            params->params = new_params;

            params->params[params->num_params].key = param_name;
            params->params[params->num_params].value = param_value;
            params->num_params++;
        } else if (strcmp(route_tokens[i], url_tokens[i]) != 0) {
            result = false;
            goto cleanup;
        }
    }

cleanup:
    for (size_t i = 0; i < num_route_tokens; i++) {
        free(route_tokens[i]);
    }
    free(route_tokens);
    for (size_t i = 0; i < num_url_tokens; i++) {
        free(url_tokens[i]);
    }
    free(url_tokens);
    return result;
}

void router_start(CometRouter* router) {
    if (!router) {
        log_message(LOG_ERROR, "Router is NULL");
        return;
    }

    router->running = true;

    while (router->running) {
        HttpcResponse* res = NULL;
        HttpcRequest* req = router_read_next_request(router);
        if (req == NULL) {
            continue;
        }

        for (size_t i = 0; i < router->num_routes; i++) {
            CometRoute* route = &router->routes[i];
            
            UrlParams params = {0};
            if (extract_url_params(route->route, req->url, &params)) {
                for (size_t j = 0; j < route->num_middleware; j++) {
                    req = route->middleware_chain[j](req, &params);
                }

                res = route->handler(req, &params);
                if (res == NULL) {
                    res = httpc_response_new("Internal Server Error", 500);
                    httpc_response_set_body(res, "500 Internal Server Error", 26);
                }
            }
            for (size_t j = 0; j < params.num_params; j++) {
                free(params.params[j].key);
                free(params.params[j].value);
            }
            if (params.params) free(params.params);
        }

        if (res == NULL) {
            res = default_not_found_handler(req);
        }

        size_t response_len = 0;
        char* response_str = httpc_response_to_string(res, &response_len);
        if (response_str == NULL) {
            log_message(LOG_ERROR, "Failed to serialize response");
            httpc_response_free(res);
            httpc_request_free(req);
            continue;
        }

        ByteCount bytes_sent = netctx_send(router->ctx, response_str, response_len);
        if (bytes_sent == SOCKET_ERROR) {
            log_message(LOG_ERROR, "Failed to send response");
        }

        free(response_str);
        httpc_request_free(req);
        httpc_response_free(res);
    }

    router_deinit(router);
}

void router_deinit(CometRouter* router) {
    if (!router) {
        log_message(LOG_ERROR, "Router is NULL");
        return;
    }

    router->running = false;
    netctx_deinit(router->ctx);

    for (size_t i = 0; i < router->num_routes; i++) {
        free(router->routes[i].route);
    }
    for (size_t i = 0; i < router->num_routes; i++) {
        free(router->routes[i].middleware_chain);
    }
    free(router->routes);
    free(router);

    log_message(LOG_INFO, "Router has been deinitialized");
}
