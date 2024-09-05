#include <WS2tcpip.h>
#include <Winsock2.h>
#include <minwindef.h>
#include <stdio.h>
#include <string.h>
#include <winnt.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
  WSADATA wsa;
  int socket_descriptor, client_socket, client_size;
  struct sockaddr_in server_addr, client_addr;
  char server_buffer[1000], client_buffer[1000];

  memset(server_buffer, '\0', sizeof(server_buffer));
  memset(client_buffer, '\0', sizeof(client_buffer));

  strcpy(server_buffer, "HTTP/1.1 200 OK\r\n"
                        "Connection: Close\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: 48\r\n"
                        "Date: Wed, 20 Jul 2016 10:55:30 GMT\r\n"
                        "Last-Modified: Tue, 19 Jul 2016 00:59:33 GMT\r\n"
                        "Server: Apache\r\n"
                        "\r\n"
                        "<html><body><h1>Hello World!</h1></body></html>");

  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    printf("Cant init wsa");
    return 0;
  }

  socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_descriptor < 0) {
    printf("Cant create a socket");
    closesocket(socket_descriptor);
    WSACleanup();
    return 0;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(3000);
  InetPton(AF_INET, __TEXT("127.0.0.1"), &server_addr.sin_addr.s_addr);

  if (bind(socket_descriptor, (struct sockaddr *)&server_addr,
           sizeof(server_addr))) {
    printf("Cant bind to a port");
    closesocket(socket_descriptor);
    WSACleanup();
    return 0;
  }

  if (listen(socket_descriptor, 1) < 0) {
    printf("Error while listening");
    closesocket(socket_descriptor);
    WSACleanup();
    return 0;
  }

  client_size = sizeof(client_addr);
  while (1) {
    client_socket = accept(socket_descriptor, (struct sockaddr *)&client_addr,
                           &client_size);

    if (client_socket < 0) {
      printf("Cant accept");
      closesocket(socket_descriptor);
      WSACleanup();
      return 0;
    }

    if (recv(client_socket, client_buffer, strlen(client_buffer), 0) < 0) {
      printf("Cant recieve");
      closesocket(socket_descriptor);
      WSACleanup();
      return 0;
    }

    if (send(client_socket, server_buffer, strlen(server_buffer), 0) < 0) {
      printf("Cant send");
      closesocket(socket_descriptor);
      WSACleanup();
      return 0;
    }

    printf("Message: %s\n", client_buffer);
  }
  closesocket(socket_descriptor);
  WSACleanup();
  return 0;
}
