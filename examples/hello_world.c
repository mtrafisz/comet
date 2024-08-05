#include <comet.h>
#include <signal.h>

HttpcResponse* hello_world_handler(HttpcRequest* req, UrlParams* _) {
    HttpcResponse* res = httpc_response_new("OK", 200);
    httpc_response_set_body(res, "Hello, world!", 13);
    httpc_add_header_v(&res->headers, "Content-Type", "text/plain");
    return res;
}

HttpcResponse* greeting_handler(HttpcRequest* req, UrlParams* params) {
    HttpcResponse* res = httpc_response_new("OK", 200);
    char* greeting = malloc(strlen(params->params[0].value) + 9);
    sprintf(greeting, "Hello, %s!", params->params[0].value);
    httpc_response_set_body(res, greeting, strlen(greeting));
    httpc_add_header_v(&res->headers, "Content-Type", "text/plain");
    free(greeting);
    return res;
}

HttpcResponse* farewell_handler(HttpcRequest* req, UrlParams* params) {
    HttpcResponse* res = httpc_response_new("OK", 200);
    char* farewell = malloc(strlen(params->params[0].value) + 12);
    sprintf(farewell, "Goodbye, %s!", params->params[0].value);
    httpc_response_set_body(res, farewell, strlen(farewell));
    httpc_add_header_v(&res->headers, "Content-Type", "text/plain");
    free(farewell);
    return res;
}

HttpcRequest* logging_middleware(HttpcRequest* req, UrlParams* _) {
    log_message(LOG_INFO, "Received %s request for %s", httpc_method_to_string(req->method), req->url);
    return req;
}

CometRouter* router;

void sigint_handler(int sig) {
    router->running = false;
    puts("");
    log_message(LOG_INFO, "Shutting down server...");
}

int main(void) {
    comet_init(false, true);
    signal(SIGINT, sigint_handler);

    router = router_init(8080);

    CometCorsConfig cors_config = COMET_CORS_DEFAULT_CONFIG;
    cors_config.allowed_methods = "GET";
    router_set_cors_policy(router, cors_config);

    router_add_route(router, "/", HTTPC_GET, hello_world_handler);
    router_add_route(router, "/hello/{name}", HTTPC_GET, greeting_handler);
    router_add_route(router, "/{name}/bye", HTTPC_GET, farewell_handler);    

    for (size_t i = 0; i < router->num_routes; i++) {
        router_add_middleware(router, i, logging_middleware);
    }

    router_start(router);

    return 0;
}
