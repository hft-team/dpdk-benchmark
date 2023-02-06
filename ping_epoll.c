#include <unistd.h>
#include <sys/types.h>    /* basic system data types */
#include <sys/socket.h>   /* basic socket definitions */
#include <netinet/in.h>   /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>    /* inet(3) functions */
#include <sys/epoll.h>    /* epoll function */
#include <fcntl.h>        /* nonblocking */
#include <sys/resource.h> /*setrlimit */
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

#define MAXEPOLLSIZE 10000
#define MAXLINE 10240
int connfd;

int handle(int connfd);
int setnonblocking(int sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK) == -1)
    {
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int servPort = 6888;
    int listenq = 1024;

    int kdpfd, nfds, n, curfds;

    struct epoll_event ev;
    struct epoll_event events[MAXEPOLLSIZE];
    char buf[MAXLINE];

    connfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));               // 每个字节都用0填充
    serv_addr.sin_family = AF_INET;                         // 使用IPv4地址
    serv_addr.sin_addr.s_addr = inet_addr("172.31.10.131"); // 具体的IP地址
    serv_addr.sin_port = htons(80);                         // 端口

    if (setnonblocking(connfd) < 0)
    {
        perror("setnonblock error");
    }

    if (connect(connfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect error");
        // return -1;
    }

    /* 创建 epoll 句柄，把监听 socket 加入到 epoll 集合里 */
    kdpfd = epoll_create(MAXEPOLLSIZE);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = connfd;
    if (epoll_ctl(kdpfd, EPOLL_CTL_ADD, connfd, &ev) < 0)
    {
        fprintf(stderr, "epoll set insertion error: fd=%d\n", connfd);
        return -1;
    }
    curfds = 1;
    char recv_buf[4096] = {0};

    for (;;)
    {
        // printf("epoll_wait start...\n");
        /* 等待有事件发生 */
        nfds = epoll_wait(kdpfd, events, curfds, 1);

        if (nfds == -1)
        {
            perror("epoll_wait");
            continue;
        }

        /* 处理所有事件 */
        for (n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == connfd)
            {
                int ret = read(connfd, recv_buf, sizeof(recv_buf));
                // printf("receivte time = %ld,ret :[%d] recv_buf:[%s]\n", now_timesatmp_us(), ret, recv_buf);
                if (recv_buf[9] != '4')
                {
                    // printf("receivte time = %ld,ret :[%d] recv_buf:[%s]\n", now_timesatmp_us(), ret, recv_buf);
                    static char send_time[17];
                    long int send_time_int;
                    memcpy(send_time, recv_buf + 154, 16);
                    send_time[16] = '\0';
                    send_time_int = strtol(send_time, NULL, 10);
                    // printf("receive send time = %ld\n", send_time_int);
                    record_lag(now_timesatmp_us() - send_time_int);
                }
            }
            // // 处理客户端请求
            // if (handle(events[n].data.fd) < 0)
            // {
            //     epoll_ctl(kdpfd, EPOLL_CTL_DEL, events[n].data.fd, &ev);
            //     curfds--;
            // }
        }

        char message[300];
        int err = -1;
        int counter = 0;
        socklen_t len = sizeof(int);

        if (reach_send_time() == 1)
        {
            if (getsockopt(connfd, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
            {
                // ff_close(events[i].data.fd);
                printf("getsockopt errno:%d %s\n", errno, strerror(errno));
                continue;
            }

            if (err != 0)
            {
                printf("not access\n");
                continue;
            }
            sprintf(message, "GET /ping?ping_time=%ld HTTP/1.1\r\nHost: 172.31.10.131\r\nContent-Type: application/json\n\r\n", now_timesatmp_us());
            int ret = send(connfd, message, strlen(message), 0);
            counter += 1;
            // printf("client fd %d, Already Send to server!, ret: %d\n", connfd, ret);
        }
    }
    close(connfd);
    return 0;
}
