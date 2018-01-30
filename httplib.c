#include "httplib.h"

int http_recv_line(int client_fd, char *buf, size_t size)
{
    if(NULL == buf)
        return HTTP_ERR;

    int i = 0;
    int nRecv = 0;
    char c = '\0';

    while((i < size - 1) && (c != NEW_LINE_CHAR)) {
        nRecv = recv(client_fd, &c, 1, 0);
        if (nRecv <= 0)
            c = NEW_LINE_CHAR;
        else {
            if (c == '\r') { // 换到本行开头
                nRecv = recv(client_fd, &c, 1, MSG_PEEK);
                if (nRecv <= 0)
                    c = NEW_LINE_CHAR;
                else if(c == NEW_LINE_CHAR){
                    nRecv = recv(client_fd, &c, 1, 0);
                }
            }

            buf[i] = c;
            ++i;
        }
    }

    buf[i] = '\0';
    return i;
}

static int parse_method(char *line, size_t length, char *method, char *uri, size_t size)
{
    char *p;
    p = strtok(line, " ");
    if(NULL == p) {
        return HTTP_ERR;
    }

    if (strcasecmp(p, "GET") == 0)
        *method = GET;

    else if (strcasecmp(p, "POST") == 0)
        *method = POST;

    else
        return HTTP_ERR;

    p = strtok(NULL, " ");
    if(NULL == p)
        return HTTP_ERR;
    printf("REAL_URI:%s\n", p);
    int path_size = strlen(p) + 1;
    strncpy(uri, p, path_size<size?path_size:size);

    return 0;
}



int parse_http_request_header(int client_fd, struct http_req_header* header)
{
#define BUFSIZE_1024 (1024)

    if (NULL == header)
        return HTTP_ERR;

    char buf[BUFSIZE_1024];
    int nRecv = 0;
    int line_no = 0;
    char method;

    while( (nRecv = http_recv_line(client_fd, buf, sizeof(buf))) != 0) {
        if (strcmp(buf, "\n") == 0)
            break;
        printf("%s", buf);
        if(0 == line_no) { //第一行parse method
            nRecv = parse_method(buf, sizeof(buf), &method, buf, sizeof(buf));
            header->method = method;

            header->uri = (char*)calloc(1, strlen(buf)+1);
            strncpy(header->uri, buf, strlen(buf)+1);
            printf("uri:%s\n", buf);
            
            printf("method:%s\n", method==GET?"GET":(method==POST?"POST":"ERR"));
        }
        ++line_no;
    }

    return 0;
}

int send_http_response_header(int client_fd, const struct http_req_header* header)
{   
    char buf[BUFSIZE_1024];
#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"
     strcpy(buf, "HTTP/1.0 200 OK\r\n");
     send(client_fd, buf, strlen(buf), 0);
     strcpy(buf, SERVER_STRING);
     send(client_fd, buf, strlen(buf), 0);

     if(strcasecmp(header->uri, "/") == 0) {
        sprintf(buf, "Content-Type: text/html\r\n");
        send(client_fd, buf, strlen(buf), 0);
     } else {
        sprintf(buf, "Content-Type: image/jpg\r\n");
        send(client_fd, buf, strlen(buf), 0);
     }
     
     strcpy(buf, "\r\n");
     send(client_fd, buf, strlen(buf), 0);

     char *path = header->uri;
     int index_html;

     if (strcasecmp(path, "/") == 0)
        index_html = open("./index.html", O_RDONLY);
     else {
        snprintf(buf, sizeof(buf), ".%s", path);
        index_html = open(buf, O_RDONLY);
        
     }
        
     
     if (-1 >= index_html) {
        printf("open err:%s\n", path);
        return -1;
     }
     int n;
     while((n = read(index_html, buf, sizeof(buf) - 1)) != 0) {
        buf[n] = '\0'; 
        send(client_fd, buf, strlen(buf), 0);
     }

     printf("send\n");
}