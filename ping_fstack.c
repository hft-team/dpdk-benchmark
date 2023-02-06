#include <unistd.h>
#include <sys/types.h>  /* basic system data types */
#include <sys/socket.h> /* basic socket definitions */
#include <netinet/in.h> /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>  /* inet(3) functions */
#include <netdb.h>      /*gethostbyname function */
#include <sys/ioctl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

#include "ff_config.h"
#include "ff_api.h"
#include "ff_epoll.h"

#include "common.h"

#define MAX_EVENTS 512
/* kevent set */
struct kevent kevSet;
/* events */
struct kevent events[MAX_EVENTS];
/* kq */
int kq;
int sockfd;
#define MAXLINE 1024
int selfclientfd;
int loop(void *arg);

#include <time.h>
#include <errno.h>

/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do
    {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

int main(int argc, char **argv)
{

    ff_init(argc, argv);

    selfclientfd = ff_socket(AF_INET, SOCK_STREAM, 0);
    printf("client:%d\n", selfclientfd);
    if (selfclientfd > 0)
    {
        int on = 1;
        ff_ioctl(selfclientfd, FIONBIO, &on);

        struct sockaddr_in server_sock;
        bzero(&server_sock, sizeof(server_sock));
        server_sock.sin_family = AF_INET;
        server_sock.sin_port = htons(80);
        if (inet_pton(AF_INET, "172.31.10.131", &server_sock.sin_addr) != 1)
        {
            printf("\nInvalid address/ Address not supported \n");
            return -1;
        }

        int ret = ff_connect(selfclientfd, (struct linux_sockaddr *)&server_sock, sizeof(struct sockaddr_in));
        if (ret < 0 && errno != EINPROGRESS)
            printf("conn failed, clientfd = %d,ret=%d,%d,%s\n", selfclientfd, ret, errno, strerror(errno));
        else
            printf("conn suc\n");
        printf("create_socket_cn clientfd = %d,ret=%d,%d,%s\n", selfclientfd, ret, errno, strerror(errno));
    }
    // EV_SET(&kevSet, clientfd, EVFILT_READ|EVFILT_WRITE, EV_ADD, 0, MAX_EVENTS, NULL);
    EV_SET(&kevSet, selfclientfd, EVFILT_WRITE | EVFILT_READ, EV_ADD, 0, MAX_EVENTS, NULL);

    assert((kq = ff_kqueue()) > 0);

    /* Update kqueue */
    ff_kevent(kq, &kevSet, 1, NULL, 0, NULL);

    ff_run(loop, NULL);
}

int loop(void *arg)
{

    unsigned nevents = ff_kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);
    unsigned i;

    int err = -1;
    socklen_t len = sizeof(int);

    int ret = ff_getsockopt(selfclientfd, SOL_SOCKET, SO_ERROR, &err, &len);
    // printf("len of sendline  = %ld", strlen(sendline));
    if (ret <= 0)
    {
        printf("ret=%d,%d,%s\n", ret, errno, strerror(errno));

        if (errno == 9)
        {
            exit(0);
        }
    };

    for (i = 0; i < nevents; ++i)
    {
        struct kevent event = events[i];
        int clientfd = (int)event.ident;
        // printf("event fd:%d\n", clientfd);
        /* Handle disconnect */
        if (event.flags & EV_EOF || event.flags & EV_ERROR)
        {
            /* Simply close socket */
            printf("event eof or error:%d,%d,%d\n", event.flags, EV_EOF, EV_ERROR);
            ff_close(clientfd);
        }
        else if (event.filter == EVFILT_WRITE && (int)event.ident == selfclientfd && ff_getsockopt(selfclientfd, SOL_SOCKET, SO_ERROR, &err, &len) == 0)
        {
            printf("receive writeable event\n");
            static char sendline[MAXLINE];
            memset(sendline, 0, MAXLINE);
            sprintf(sendline, "GET /ping?ping_time=%ld HTTP/1.1\r\n", now_timesatmp_us());
            strcat(sendline, "Host: 172.31.10.131\r\n");
            strcat(sendline, "Content-Type: application/json\n");
            strcat(sendline, "\r\n");

            int err = -1;
            socklen_t len = sizeof(int);
            // printf("len of sendline  = %ld", strlen(sendline));
            // ff_getsockopt(selfclientfd, SOL_SOCKET, SO_ERROR, &err, &len);
            ssize_t n = ff_write(selfclientfd, sendline, strlen(sendline) - 1);
            printf("write bytes = %ld, msg = %s", n, sendline);
        }
        else if (event.filter == EVFILT_READ)
        {

            char buf[256];
            size_t readlen = ff_read((int)event.ident, buf, sizeof(buf));
            printf("time:%ld receive msg:%s\n", now_timesatmp_us(), buf);
        }
        else
        {
            printf("unknown event: %8.8X\n", event.flags);
        }
    }
}
