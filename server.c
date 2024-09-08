#include <WS2tcpip.h>
#include <Winsock2.h>
#include <minwindef.h>
#include <stdio.h>
#include <string.h>
#include <winnt.h>
#include <winsock2.h>
#define MAX_RESPONSE_SIZE 1024

#pragma comment(lib, "ws2_32.lib")

typedef struct http_server {
  int server_socket;
} http_server;

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
    return 0;
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

  snprintf(response, MAX_RESPONSE_SIZE,
           "HTTP/1.1 %d %s\r\n"
           "Connection: close\r\n"
           "Content-Type: %s\r\n"
           "Content-Length: %d\r\n"
           "\r\n"
           "%s",
           response_code, response_code_text, content_type, (int)strlen(body),
           body);

  printf("%s", response);
}

void start_server(http_server server) {

  int client_socket, client_size;
  struct sockaddr_in client_addr;
  char server_buffer[1000], client_buffer[1000];

  memset(server_buffer, '\0', sizeof(server_buffer));
  memset(client_buffer, '\0', sizeof(client_buffer));

  strcpy(server_buffer, "HTTP/1.1 200 OK\r\n"
                        "Connection: close\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: 48\r\n"
                        "\r\n"
                        "<html><body><h1>Hello World!</h1></body></html>");

  if (listen(server.server_socket, 1) < 0) {
    printf("Error while listening");
    closesocket(server.server_socket);
    WSACleanup();
    return;
  }

  client_size = sizeof(client_addr);
  while (1) {
    client_socket = accept(server.server_socket,
                           (struct sockaddr *)&client_addr, &client_size);

    if (client_socket < 0) {
      printf("Cant accept");
      closesocket(client_socket);
      WSACleanup();
      return;
    }

    if (recv(client_socket, client_buffer, strlen(client_buffer), 0) < 0) {
      printf("Cant recieve");
      closesocket(client_socket);
      WSACleanup();
      return;
    }

    char response[MAX_RESPONSE_SIZE];
    create_response(response, 200,
                    "<html><body><h1>Hello World!</h1></body></html>", 1);
    if (send(client_socket, response, strlen(response), 0) < 0) {
      printf("Cant send");
      closesocket(client_socket);
      WSACleanup();
      return;
    }

    printf("Message: %s\n", client_buffer);
  }
}

void close_server(http_server *server) {
  closesocket(server->server_socket);
  WSACleanup();
}

int main() {
  http_server app;

  app = create_server(3000);
  start_server(app);
  close_server(&app);

  return 0;
}
