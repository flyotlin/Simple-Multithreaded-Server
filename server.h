#ifndef SERVER_H_
#define SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <pthread.h>

#include "threadpool.h"

#define TRUE 1
#define FALSE 0
#define MAX_BUFFER_SIZE 4096
#define THREAD_MAX_SIZE 10
#define MAX_CONNECTION 50

/**
 *  Headers for different types of contents
 */
#define HTML_HEADER "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nCharset: UTF-8\r\n\r\n"
#define CSS_HEADER "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\ncharset=UTF-8\r\n\r\n"
#define JS_HEADER "HTTP/1.1 200 OK\r\nContent-Type: script\r\ncharset:UTF-8\r\n\r\n"
#define PNG_HEADER "HTTP/1.1 200 OK\r\nContent-Type: image/png\n\rConnection: keep-alive\r\naccept-ranges: bytes\r\ncharset=UTF-8\r\ncontent-length: "
#define NOTFOUND_HEADER "HTTP/1.1 404\r\n"
#define DEFAULT_HEADER "HTTP/1.1 200 OK\r\ncharset=UTF-8\r\n\r\n"

/* Port number specified */
extern size_t port;

typedef struct {
        pthread_t tid;
        size_t client_fd;
        int isAvailable;
} client_thread;
client_thread client_threads_pool[THREAD_MAX_SIZE];

/**
 *  File descriptor for server, client socket
 */
struct conn_fd {
        size_t server_fd;   
        size_t client_fd;
} connFd;

struct sockaddr_in server_sock, client_sock;

/**
 *  Store the info about the request header 
 */
#define MAX_CharArr_SIZE 100
struct request_header {
        char http_method[MAX_CharArr_SIZE];
        char url[MAX_CharArr_SIZE];
        char req_file_type[MAX_CharArr_SIZE];
        char *filepath;
};


/* The server asset folder directory path */
extern const char* PUBLIC_PATH;

/* Run the server, start the response loop */
void run_server();

/* Create socket */
void create_socket();

/* Bind and listen on port 8080 */
void init_socket();

/* Handles the http response from the client */
void http_response_handler(void *);

/* Init the threadpool and start the server loop */
void server_routine();

/* Parse the request, store the req info into struct request_header */
void parse_request(struct request_header *, char *);

/* Create a http response header accord. to file type */
char *create_response_header(struct request_header *);

/* Respond http header */
void write_http_header(size_t, char *);

/* Respond http body */
void write_http_body(size_t, struct request_header *);

#endif  // SERVER_H_