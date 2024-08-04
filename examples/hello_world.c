#include <comet.h>
#include <signal.h>

HttpcResponse* hello_world_handler(HttpcRequest* req) {
    HttpcResponse* res = httpc_response_new("OK", 200);
    httpc_response_set_body(res, "Hello, world!", 13);
    httpc_add_header_v(&res->headers, "Content-Type", "text/plain");
    return res;
}

HttpcRequest* logging_middleware(HttpcRequest* req) {
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
    int route_id = router_add_route(router, "/", HTTPC_GET, hello_world_handler);
    router_add_middleware(router, route_id, logging_middleware);

    router_start(router);

    return 0;
}
