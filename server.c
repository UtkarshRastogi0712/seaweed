#include <WS2tcpip.h>
#include <Winsock2.h>
#include <minwindef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winnt.h>
#include <winsock2.h>

#define MAX_SIZE 1024
#pragma comment(lib, "ws2_32.lib")

typedef enum http_request_type { get, put, post, delete } http_request_type;

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

int pattern_match(char *source, char *pattern, int index) {
  int source_len = strlen(source);
  int pattern_len = strlen(pattern);
  for (int i = index; i < source_len - pattern_len + 1; i++) {
    int j = 0;
    while (source[i + j] == pattern[j] && j < pattern_len) {
      j += 1;
      if (j == pattern_len) {
        return i;
      }
    }
  }
  return -1;
}

int send_TCP(int socket, char *buffer, int length) {
  while (length > 0) {
    int sent = send(socket, buffer, length, 0);
    if (sent < 1)
      return -1;
    buffer += sent;
    length -= sent;
  }
  return 0;
}

int recv_TCP(int socket, char *buffer, int length) {
  while (length > 0) {
    int received = recv(socket, buffer, length, 0);
    if (received < 0)
      return -1;
    else if (received == 0)
      return 0;
    else if (pattern_match(buffer, "\r\n\r\n", 0) >= 0)
      return 0;
    buffer += received;
    length -= received;
  }
  return -1;
}

int create_server_socket(int port) {
  WSADATA wsa;
  int socket_descriptor;
  struct sockaddr_in server_addr;

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  InetPton(AF_INET, __TEXT("127.0.0.1"), &server_addr.sin_addr.s_addr);

  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    printf("Cant init wsa");
    return -1;
  }

  socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_descriptor < 0) {
    printf("Cant create a socket");
    closesocket(socket_descriptor);
    WSACleanup();
    return -1;
  }

  if (bind(socket_descriptor, (struct sockaddr *)&server_addr,
           sizeof(server_addr))) {
    printf("Cant bind to a port");
    closesocket(socket_descriptor);
    WSACleanup();
    return -1;
  }

  return socket_descriptor;
}

http_server create_server(int port) {
  http_server server;
  server.server_socket = create_server_socket(port);
  server.endpoint_count = 0;
  return server;
}

void create_response(char *response_string, http_response *response) {
  char *response_code_text;
  char *content_type;

  switch (response->response_code) {
  case 200:
    response_code_text = "OK";
    break;
  case 400:
    response_code_text = "Bad Request";
    break;
  case 404:
    response_code_text = "Not Found";
    break;
  case 500:
    response_code_text = "Internal Server Error";
    break;
  }

  switch (response->content_type_code) {
  case 0:
    content_type = "text/plain";
    break;
  case 1:
    content_type = "text/html";
    break;
  case 2:
    content_type = "application/json";
    break;
  }

  snprintf(response_string, MAX_SIZE,
           "HTTP/1.1 %d %s\r\n"
           "Connection: close\r\n"
           "Content-Type: %s\r\n"
           "Content-Length: %d\r\n"
           "\r\n"
           "%s",
           response->response_code, response_code_text, content_type,
           (int)strlen(response->body), response->body);
}

void parse_request(http_request *request, char *client_buffer) {
  if (pattern_match(client_buffer, "GET", 0) == 0) {
    request->requet_type = get;
  } else if (pattern_match(client_buffer, "POST", 0) == 0) {
    request->requet_type = post;
  } else if (pattern_match(client_buffer, "PUT", 0) == 0) {
    request->requet_type = put;
  } else if (pattern_match(client_buffer, "DELETE", 0) == 0) {
    request->requet_type = delete;
  } else {
    request->requet_type = get;
  }

  int url_start = pattern_match(client_buffer, " ", 0);
  int url_end = pattern_match(client_buffer, " ", url_start + 1);
  snprintf(request->url, url_end - url_start, "%s",
           client_buffer + url_start + 1);

  if (pattern_match(client_buffer, "Content-Length", 0) >= 0) {
    int content_header = pattern_match(client_buffer, "Content-Length", 0);
    int body_start =
        pattern_match(client_buffer, "\r\n\r\n", content_header) + 4;
    snprintf(request->body, strlen(client_buffer) - body_start + 1, "%s",
             client_buffer + body_start);
  } else {
    snprintf(request->body, 0, "%s", "\0");
  }
  return;
}

void add_endpoint(http_server *socket, http_request_type request_type,
                  char *url,
                  void (*endpoint_function)(http_request *, http_response *)) {
  http_endpoint *endpoint = malloc(sizeof(http_endpoint));
  socket->endpoints[socket->endpoint_count] = endpoint;
  socket->endpoint_count++;

  snprintf(endpoint->url, strlen(url) + 1, "%s", url);
  endpoint->request_type = request_type;
  endpoint->endpoint_function = endpoint_function;
}

void start_server(http_server *server) {

  int client_size;
  struct sockaddr_in client_addr;
  char client_buffer[MAX_SIZE];

  memset(client_buffer, '\0', sizeof(client_buffer));

  if (listen(server->server_socket, 5) < 0) {
    printf("Error while listening");
    closesocket(server->server_socket);
    WSACleanup();
    return;
  }

  client_size = sizeof(client_addr);
  while (1) {
    int client_socket;
    memset(client_buffer, '\0', sizeof(client_buffer));

    if ((client_socket =
             accept(server->server_socket, (struct sockaddr *)&client_addr,
                    &client_size)) < 0) {
      wprintf(L"Cant accept%ld\n", WSAGetLastError());
      closesocket(client_socket);
      continue;
    }

    char response_string[MAX_SIZE];
    if (recv_TCP(client_socket, client_buffer, MAX_SIZE) < 0) {
      printf("Cant recieve");
      closesocket(client_socket);
      continue;
    }

    if (pattern_match(client_buffer, "HTTP/1.1", 0) != -1) {
      http_request *request = malloc(sizeof(http_request));
      parse_request(request, client_buffer);
      printf("\nVerb: %d\nURL: %s\nBody:\n %s\n\n", request->requet_type,
             request->url, request->body);

      int flag = 0;

      for (int i = 0; i < server->endpoint_count; i++) {
        if (pattern_match(request->url, server->endpoints[i]->url, 0) == 0 &&
            strlen(request->url) == strlen(server->endpoints[i]->url) &&
            server->endpoints[i]->request_type == request->requet_type) {
          http_response *response = malloc(sizeof(http_response));
          server->endpoints[i]->endpoint_function(request, response);
          create_response(response_string, response);
          flag++;
          break;
        }
      }
      if (flag == 0) {
        http_response *response = malloc(sizeof(http_response));
        response->response_code = 404;
        response->content_type_code = 1;
        char response_body[] =
            "<html><head><title>Not Found</title></head><body><h1>404:Not "
            "Found</h1></body></html>";
        snprintf(response->body, strlen(response_body), "%s", response_body);
        create_response(response_string, response);
      }
    } else {
      printf("Not an http request");
      closesocket(client_socket);
      continue;
    }

    if (send_TCP(client_socket, response_string, strlen(response_string)) < 0) {
      printf("Cant send");
      closesocket(client_socket);
      continue;
    }
    printf("----------------------------------");
  }
}

void close_server(http_server *server) {
  printf("Closing socket\n");
  closesocket(server->server_socket);
  WSACleanup();
}

void test_endpoint(http_request *request, http_response *response) {
  response->response_code = 200;
  response->content_type_code = 1;
  char response_body[] =
      "<html><head><title>Endpoint</title></head><body><h1>Hello "
      "World from endpoint</h1></body></html>";
  snprintf(response->body, strlen(response_body), "%s", response_body);
  return;
}

int main() {
  http_server app;

  app = create_server(3000);

  add_endpoint(&app, get, "/hello/world", test_endpoint);
  start_server(&app);
  close_server(&app);

  return 0;
}
