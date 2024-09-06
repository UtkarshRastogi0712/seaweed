#include <WS2tcpip.h>
#include <Winsock2.h>
#include <minwindef.h>
#include <stdio.h>
#include <string.h>
#include <winnt.h>
#include <winsock2.h>

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

void start_server() {}

int main() {
  http_server app;
  app = create_server(3000);

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

  if (listen(app.server_socket, 1) < 0) {
    printf("Error while listening");
    closesocket(app.server_socket);
    WSACleanup();
    return 0;
  }

  client_size = sizeof(client_addr);
  while (1) {
    client_socket = accept(app.server_socket, (struct sockaddr *)&client_addr,
                           &client_size);

    if (client_socket < 0) {
      printf("Cant accept");
      closesocket(client_socket);
      WSACleanup();
      return 0;
    }

    if (recv(client_socket, client_buffer, strlen(client_buffer), 0) < 0) {
      printf("Cant recieve");
      closesocket(client_socket);
      WSACleanup();
      return 0;
    }

    if (send(client_socket, server_buffer, strlen(server_buffer), 0) < 0) {
      printf("Cant send");
      closesocket(client_socket);
      WSACleanup();
      return 0;
    }

    printf("Message: %s\n", client_buffer);
  }
  closesocket(app.server_socket);
  WSACleanup();
  return 0;
}
