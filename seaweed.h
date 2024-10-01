#ifndef SEAWEED_H
#define SEAWEED_H

#define MAX_SIZE 1024

typedef enum http_request_type {
  http_get,
  http_put,
  http_post,
  http_delete
} http_request_type;

typedef struct http_request {
  char url[MAX_SIZE];
  char body[MAX_SIZE];
  http_request_type requet_type;
} http_request;

typedef struct http_response {
  int response_code;
  char body[MAX_SIZE];
  int content_type_code;
} http_response;

typedef struct http_endpoint {
  http_request_type request_type;
  char url[MAX_SIZE];
  void (*endpoint_function)(http_request *, http_response *);
} http_endpoint;

typedef struct http_server {
  int server_socket;
  http_endpoint *endpoints[MAX_SIZE];
  int endpoint_count;
} http_server;

http_server create_server(int port);

void add_endpoint(http_server *socket, http_request_type request_type,
                  char *url,
                  void (*endpoint_funcion)(http_request *, http_response *));

void start_server(http_server *server);

void close_server(http_server *server);

#endif
