#include "seaweed.h"
#include <stdio.h>
#include <string.h>

void test_endpoint(http_request *request, http_response *response) {
  response->response_code = 200;
  response->content_type_code = 1;
  char response_body[] =
      "<html><head><title>Endpoint</title></head><body><h1>Hello "
      "World from endpoint</h1></body></html>";
  snprintf(response->body, strlen(response_body), "%s", response_body);
  snprintf(response->body, strlen(response_body) + 1, "%s", response_body);
  return;
}
void post_endpoint(http_request *request, http_response *response) {
  response->response_code = 400;
  response->content_type_code = 2;
  char response_body[] = "{\"hello\" : \"world\"}";
  snprintf(response->body, strlen(response_body) + 1, "%s", response_body);
  return;
}

int main() {
  http_server app;

  app = create_server(3000);

  add_endpoint(&app, http_get, "/hello/world", test_endpoint);
  add_endpoint(&app, http_post, "/resource", post_endpoint);
  start_server(&app);
  close_server(&app);

  return 0;
}
