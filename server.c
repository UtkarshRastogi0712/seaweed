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

typedef struct http_endpoint {
  http_request_type request_type;
  char url[MAX_SIZE];
  void (*endpoint)(http_request);
} http_endpoint;

typedef struct http_server {
  int server_socket;
  http_endpoint *endpoints[MAX_SIZE];
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
    else if (buffer[strlen(buffer) - 1] == '\n' &&
             buffer[strlen(buffer) - 2] == '\r')
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
  return server;
}

void create_response(char *response, int response_code, char *body,
                     int content_type_code) {
  char *response_code_text;
  char *content_type;

  switch (response_code) {
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

  switch (content_type_code) {
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

  snprintf(response, MAX_SIZE,
           "HTTP/1.1 %d %s\r\n"
           "Connection: close\r\n"
           "Content-Type: %s\r\n"
           "Content-Length: %d\r\n"
           "\r\n"
           "%s",
           response_code, response_code_text, content_type, (int)strlen(body),
           body);
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
  snprintf(request->url, url_end - url_start + 1, "%s",
           client_buffer + url_start);

  if (pattern_match(client_buffer, "Content-Length", 0) >= 0) {
    int content_header_start =
        pattern_match(client_buffer, "Content-Length", 0);
    int content_header_end =
        pattern_match(client_buffer, "\r\n", content_header_start);
    char *end = client_buffer + content_header_end - 1;
    int content_length =
        strtol(client_buffer + content_header_start + 15, &end, 10);

    snprintf(request->body, content_length + 3, "%s",
             client_buffer + content_header_end + 2);
  } else {
    snprintf(request->body, 0, "%s", "\0");
  }
  return;
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

    if ((client_socket =
             accept(server->server_socket, (struct sockaddr *)&client_addr,
                    &client_size)) < 0) {
      wprintf(L"Cant accept%ld\n", WSAGetLastError());
      closesocket(client_socket);
      continue;
    }

    char response[MAX_SIZE];
    if (recv_TCP(client_socket, client_buffer, MAX_SIZE) < 0) {
      printf("Cant recieve");
      closesocket(client_socket);
      continue;
    }

    printf("Message: %s\n", client_buffer);

    http_request *request = malloc(sizeof(http_request));
    parse_request(request, client_buffer);
    printf("\n Verb: %d, URL: %s , Body: %s\n\n", request->requet_type,
           request->url, request->body);

    if (pattern_match(client_buffer, "HTTP/1.1", 0) != -1) {
      create_response(response, 200,
                      "<html><body><h1>Hello World!</h1></body></html>", 1);
    } else {
      create_response(response, 400,
                      "<html><body><h1>Bad Request </h1></body></html>", 1);
    }

    if (send_TCP(client_socket, response, strlen(response)) < 0) {
      printf("Cant send");
      closesocket(client_socket);
      continue;
    }
  }
}

void close_server(http_server *server) {
  printf("Closing socket\n");
  closesocket(server->server_socket);
  WSACleanup();
}

int main() {
  http_server app;

  app = create_server(3000);
  start_server(&app);
  close_server(&app);

  return 0;
}
