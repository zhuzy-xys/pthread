//
// Created by 振宇 on 2017/12/23.
//

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


#define SOCKET_SUC        (0)         /* socket success */
#define SOCKET_ERR        (-1)         /* socket err */
#define SOCKET_BIND_ERR   (-2)  /* socket bind err */
#define SOCKET_LISTEN_ERR (-3)  /* socket listen err */
#define MAX_IPV4_LEN      (16)
#define BLOCK_BUF_SIZE    (1024)

int create_tcp_socket(const char *address, unsigned short port);
int recv_tcp_data(int fd, char **data, int *len);
int set_fd_unblock(int fd);

int serv_socket = -1;

typedef struct
{
    int buf_len;
    int buf_flu;
    char *buf;
    char data[0];
} recv_buf;



void* pthread_print(void *data);

void __free__()
{
    if (serv_socket >= 0) {
        printf("close fd:%d\n", serv_socket);
        close(serv_socket);
    }
        
}

int set_sock_reuse(int sock)
{
    if(sock < 0)
        return SOCKET_ERR;

    int on = 1;
    if ((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0)
    {
        printf("msg:%s\n", strerror(errno));
        return SOCKET_ERR;
    }

    return SOCKET_SUC;
}

inline static void signal_handler(int signo)
{
    printf("recv a signal:%d\n", signo);
    switch(signo)
    {
        case SIGINT:
            __free__();
            exit(1);
            break;
        case SIGQUIT:
            __free__();
            exit(1);
            break;
        default:
            break;
    }
}

int main(int argc, char **argv)
{
    int nRet, accept_fd, recv_len;
    char *buf;

    //注册中断信号
	//signal(SIGINT, signal_handler);
    //注册中断信号
	//signal(SIGQUIT, signal_handler);
    
    nRet = create_tcp_socket("192.168.0.35", 11111);
    
    if (nRet != SOCKET_SUC)
    {
        __free__();
        exit(1);
    }
    printf("start recv\n");

    while (1) {
        recv_tcp_data(serv_socket, &buf, &recv_len);
        printf("msg:%s\n", buf);
    }

    
    __free__();
    //free(&buf);
    return 0;
}

int recv_tcp_data(int fd, char **data, int *len)
{
    if(fd < 0 || *data == NULL || len == NULL)
        goto ERR_INPUT;

    struct sockaddr_in cli_addr;
    socklen_t addr_len;
    int cli_fd = -1;
    int data_len, buf_len, curr_buf_len = 0 /*, max_buf */;
    char ipv4[MAX_IPV4_LEN];
    /* char *buf; */
    recv_buf buf;
    
    cli_fd = accept(fd, (struct sockaddr*)&cli_addr, &addr_len);
        
    /* ip long to string */
     
    if (inet_ntop(AF_INET, (void*)&cli_addr.sin_addr.s_addr, ipv4, sizeof(ipv4)) < 0)
        goto ERR_SOCKET;
    
    printf("client addr->ip:%s, port:%d\n", ipv4, ntohs(cli_addr.sin_port));
    if (cli_fd < 0)
        goto ERR_SOCKET;

    //buf = (char*)calloc(1, BLOCK_BUF_SIZE);
    memset(&buf, 0, sizeof(recv_buf));
    buf.buf_flu = BLOCK_BUF_SIZE;
    buf.buf_len = BLOCK_BUF_SIZE;
    buf.buf = (char*)calloc(1, BLOCK_BUF_SIZE);
    
    //max_buf  = BLOCK_BUF_SIZE;
    //set_fd_unblock(cli_fd);
    
    while(1) {
        buf_len = recv(cli_fd, &buf.buf[curr_buf_len], BLOCK_BUF_SIZE, 0);
        printf("buf len:%u\n", buf_len);
        curr_buf_len += buf_len;
        if (buf_len < 0)
            goto ERR_SOCKET;
        else if (buf_len == 0)
            break;
        else {
            buf.buf_flu -= buf_len;
            if (buf.buf_flu <= 0) {
                printf("before realloc:%d, realloc:%d", buf.buf_len, buf.buf_len + BLOCK_BUF_SIZE);
                buf.buf = (char*)realloc(buf.buf, buf.buf_len + BLOCK_BUF_SIZE);
                buf.buf_len += BLOCK_BUF_SIZE;
                buf.buf_flu += BLOCK_BUF_SIZE;
            }
        }
        if(buf_len < BLOCK_BUF_SIZE);
            break;
    }

    *data = buf.buf;
    *len  = buf.buf_len - buf.buf_flu;

    close(cli_fd);
    return SOCKET_SUC;

ERR_INPUT:
    printf("input error: fd<0(%s), *data==NULL(%s)", 
        fd<0?"true":"false",
        *data == NULL?"true":"false");
    return SOCKET_ERR;

ERR_SOCKET:
    printf("msg:%s\n", strerror(errno));
    if (cli_fd >= 0)
        close(cli_fd);
    return SOCKET_ERR;
}

int create_tcp_socket(const char *address, unsigned short port)
{
    struct sockaddr_in addr;
    int nRet;
    
    if((serv_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        goto ERR;
    }
    nRet = set_sock_reuse(serv_socket);
    if (nRet != SOCKET_SUC)
        goto ERR;
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    if (NULL == address) 
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    else {
        //绑定只有IP，不能绑定域名
        nRet = inet_pton(AF_INET, address, &addr.sin_addr);
        if (nRet != 1)
            goto ERR;

    }
    
    addr.sin_port = htons(port);

    if  (bind(serv_socket, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
        goto ERR;
    }

    if (listen(serv_socket, SOMAXCONN) < 0) {
        goto ERR;
    }
    
    return 0;

ERR:
    close(serv_socket);
    printf("msg:%s\n", strerror(errno));
    return SOCKET_ERR;
}

int set_fd_unblock(int fd)
{
    if(fd < 0)
        return -1;

    int flag = fcntl(fd, F_GETFL, 0);
    if (flag < 0)
        goto ERR;

    flag = flag | O_NONBLOCK;
    
    flag = fcntl(fd, F_SETFL, flag);
    if (flag < 0)
        goto ERR;

    return 0;

ERR:
    printf("msg:%s\n", strerror(errno));
    return SOCKET_ERR;
}

void* pthread_print(void *data)
{
    int i;
    for (int i = 0; i < 10; ++i) {
        printf("this %lu thread\n", pthread_self());
        sleep(3);
    }
}
