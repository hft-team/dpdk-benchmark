#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

#include "ff_config.h"
#include "ff_api.h"
#include "ff_epoll.h"

#include "common.h"

int epfd;
int client_socket;
#define MAX_EVENTS 512

int loop()
{
    int ret = 0;
    char recv_buf[4096] = {0};
    char message[300];

    struct epoll_event events[MAX_EVENTS];
    int nevents = ff_epoll_wait(epfd, events, MAX_EVENTS, -1);
    int i;

    // if (nevents == 0)
    // {
    //     //  printf("nevents: %d\n", nevents);
    //     return -1;
    // }

    for (i = 0; i < nevents; i++)
    {
        int err = -1;
        socklen_t len = sizeof(int);

        if (events[i].events & EPOLLOUT)
        {
            if (ff_getsockopt(events[i].data.fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
            {
                ff_close(events[i].data.fd);
                printf("getsockopt errno:%d %s\n", errno, strerror(errno));
                return -1;
            }

            if (err != 0)
            {
                printf("not access\n");
                return -1;
            }
        }
        else if (events[i].events & EPOLLIN)
        {
            ret = ff_read(events[i].data.fd, recv_buf, sizeof(recv_buf));
            // printf("receivte time = %ld,ret :[%d] recv_buf:[%s]\n", now_timesatmp_us(), ret, recv_buf);
            if (recv_buf[9] != '4')
            {
                // printf("receivte time = %ld,ret :[%d] recv_buf:[%s]\n", now_timesatmp_us(), ret, recv_buf);
            }

            static char send_time[17];
            long int send_time_int;
            memcpy(send_time, recv_buf + 154, 16);
            send_time[16] = '\0';
            send_time_int = strtol(send_time, NULL, 10);
            long int now = now_timesatmp_us();
            printf("now = %ld,send_time_str = %s, receive send time = %ld, cost = %ld\n", now, send_time, send_time_int, now - send_time_int);
            record_lag(now - send_time_int);

            // ff_close(events[i].data.fd);
        }
    }

    int err = -1;
    socklen_t len = sizeof(int);

    if (ff_getsockopt(client_socket, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
    {
        ff_close(client_socket);
        printf("getsockopt errno:%d %s\n", errno, strerror(errno));
        return -1;
    }

    if (err != 0)
    {
        printf("not access\n");
        return -1;
    }
    // printf("test reach time\n");
    int reach_time = reach_send_time();
    if (reach_time == 1)
    {
        sprintf(message, "GET /ping?ping_time=%ld HTTP/1.1\r\nHost: 172.31.10.131\r\nContent-Type: application/json\n\r\n", now_timesatmp_us());
        ret = ff_send(client_socket, message, strlen(message), 0);
        // printf("client fd %d, Already Send to server!, ret: %d\n", client_socket, ret);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;

    ff_init(argc, argv);

    client_socket = ff_socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        printf("ff_socket failed\n");
        exit(1);
    }

    int on = 1;
    ff_ioctl(client_socket, FIONBIO, &on); /* unblock */

    epfd = ff_epoll_create(1024);

    struct epoll_event ev;
    ev.data.fd = client_socket;
    ev.events = EPOLLOUT | EPOLLIN | EPOLLET;
    ff_epoll_ctl(epfd, EPOLL_CTL_ADD, client_socket, &ev);

    if (inet_pton(AF_INET, "172.31.10.131", &server_addr.sin_addr) != 1)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    server_addr.sin_port = htons(80);

    if (ff_connect(client_socket, (struct linux_sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("connect failed: %s\n", strerror(errno));
    }

    ff_run(loop, NULL);
    return 0;
}