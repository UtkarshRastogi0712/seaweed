# Seaweed

## Introduction
Seaweed is a lightweight single threaded Http library written in C for Windows machines. It is built along the lines of ExpressJs and other popular Http Frameworks that provide a simple interface for creating an Http 1.1 Server. The goal behind Seaweed was to create an Http Server from scratch to gain a deeper understanding of how the Http protocol works and to understand how larger applications are written and structured in a low level languaage like C.

## Features
Seaweed is a fairly lightweight library with a set of simple features:
- Create a single-threaded Http server using TCP sockets.
- Allow creation of endpoints to interact with incoming request.
- Access Http request URL, Verb and body.
- Send Http responses with custom status codes, resource type and body.

## Setup
- Clone the repository.
- Copy the seaweed.h and seaweed.c file into the working directory
- Include the seaweed header file into your C code
  ```c
  #include "seaweed.h"
  ```
- Compile to object code using your preferred compiler
  ```
  gcc server.c seaweed.c
  clang server.c seaweed.c
  ```

## Usage
- Import the library
  ```c
  #include "seaweed.h"
  ```
  
- Create an http_server struct on port of choice to inintialise the soket
  ```c
  http_server app = create_server(3000)
  ```
  
- Add an endpoint to invoke a function (passed using a function pointer)
  ```c
  //pass the address of http_struct, the http verb, full url of endpoint, and the function to be called
  add_endpoint(&app, http_get, "/home", home);
  ```
  
- Create the function to be called (Takes an http_request and http_response pointer as arguments)
  ```c
  void home(http_request *request, http_response *response){
    //HTTP 200 OK
    response->response_code = 200;
  
    //Plain text
    response->content_type_code = 0;
  
    //Send a plaintext response
    snprintf( response->body, strlen("Hello World"), "%s", "Hello World");
  
    return;
  }
  
- Start the server
  ```c
  start_server(&app);
  ```
  
- Close the server
  ```c
  close_server(&app);

## Scope for Improvement
- Add a multi-threaded handler for incoming requests
- Parse Http headers from the incoming TCP buffer
- Allow adding custom headers to the outgoing response
- Add support for UNIX socket
- Access path variables in the Http request
- Improve data structure to store endpoints.


  
