#include <stdio.h>

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#ifndef __HTTP_LIB_H_
#define __HTTP_LIB_H_

#define NEW_LINE_CHAR '\n'
#define HTTP_ERR (-1)

#define GET  (1)
#define POST (2) //垃圾代码，目前支持这两个

struct http_req_header
{   
    int content_length;
    char *uri;
    char method;
};

int http_recv_line(int client_fd, char *buf, size_t size);
int parse_http_request_header(int client_fd, struct http_req_header* header);
int send_http_response_header(int client_fd, const struct http_req_header* header);


#endif

