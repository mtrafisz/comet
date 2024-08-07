# COMET - simple, single-threaded, http/1.1 web server (framework?)

Written 100% in C. With spite in my heart.

## `WARNING`

This is a learning project, and is not intended for any serious use. It is tested only for basic leaks and crashes, and is not optimized in any way.

You are welcome to contibute, though, if You want to change that.

## Building and Including in Your Project

Intended building process uses `cmake`. To build static library, run:

```bash
cmake -S . -B build && cmake --build build
```

In root directory of the project. This will create `libcomet.a` in `build` directory.

To include in your project, add `comet.h` to your project and link against `libcomet.a`.

```bash
gcc -o your_target your_target.c -Lpath/to/comet/build -lcomet -Ipath/to/comet/include
```

----

You can also use this library by adding it as subfolder in your CMakeLists.txt:

```cmake
add_subdirectory(path/to/comet)

target_link_libraries(your_target comet)
target_include_directories(your_target PUBLIC path/to/comet/include)
```

## Usage

Main feature of this library is `CometRouter` structure. It is used to define routes and their handlers, as well as add middlewares.

Http parsing and creation is done using [httpc](https://github.com/mtrafisz/httpc) library, that is already included in comet as dependency.

This is a simple example of how to use this library:

```c
#include <comet.h>
#include <signal.h>

HttpcResponse* get_hello(void* state, HttpcRequest* request, UrlParams* params) {
    HttpcResponse* response = NULL;
    char* name = NULL;

    for (int i = 0; i < params->num_params; i++) {
        if (strcmp(params->params[i].key, "name") == 0) {
            name = params->params[i].value;
            break;
        }
    }

    if (name == NULL) {
        response = httpc_response_new("Bad Request", 400);
        httpc_response_set_body(response, "Name parameter is required", 26);
    } else {
        response = httpc_response_new("OK", 200);

        char* body = malloc(strlen(name) + 7);
        sprintf(body, "Hello %s!", name);
        httpc_response_set_body(response, body, strlen(body));
        free(body);
    }

    httpc_add_header_v(&response->headers, "Content-Type", "text/plain");
    return response;
}

CometRouter* router;

void handle_sigint(int _) {
    router->running = 0;
    puts("");
}

int main(void) {
    signal(SIGINT, handle_sigint);

    router = router_init(8080, NULL);
    if (router == NULL) {
        return 1;
    }

    router_add_route(router, "/hello/{name}", HTTPC_GET, get_hello);

    router_start(router);

    return 0;
}
```

More in [examples](examples) directory or in [this project](https://github.com/mtrafisz/shortener)

Detailed documentation is not available yet. There are some doxygen comments in the code, but almost nothing is finallized yet.

## TODO

- [ ] Implement multithreading
- [ ] `router_add_static_dir` function for serving static files quickly
- [ ] Detailed documentation (doxygen?)
- [ ] More examples (with luatemplate, sqlite, etc.)

