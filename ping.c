#include <unistd.h>
#include <sys/types.h>  /* basic system data types */
#include <sys/socket.h> /* basic socket definitions */
#include <netinet/in.h> /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>  /* inet(3) functions */
#include <netdb.h>      /*gethostbyname function */
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#define MAXLINE 1024

void record_lag(long int lag)
{
    static long int cost[100];
    static int counter = 0;
    if (counter == 100)
    {
        long int all_cost = 0;
        for (int i = 0; i < 100; i++)
        {
            all_cost += cost[i];
        }
        printf("mean of cost = %ld", all_cost / 100);
        counter += 1;
        return;
    }
    cost[counter] = lag;
    counter++;
    return;
}

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

long int now_timesatmp_us()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

int handle(int connfd);

int main(int argc, char **argv)
{
    char buf[MAXLINE];
    int connfd;
    struct sockaddr_in servaddr;

    connfd = socket(AF_INET, SOCK_STREAM, 0);
    // 向服务器（特定的IP和端口）发起请求
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));               // 每个字节都用0填充
    serv_addr.sin_family = AF_INET;                         // 使用IPv4地址
    serv_addr.sin_addr.s_addr = inet_addr("172.31.10.131"); // 具体的IP地址
    serv_addr.sin_port = htons(80);                         // 端口

    if (connect(connfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect error");
        return -1;
    }
    printf("welcome to echoclient\n");
    handle(connfd); /* do it all */
    close(connfd);
    printf("exit\n");
    exit(0);
}

int handle(int sockfd)
{

    char sendline[MAXLINE], recvline[MAXLINE];
    int n;
    for (int i = 0; i < 200; i++)
    {

        msleep(100);
        long int send_time = now_timesatmp_us();
        memset(sendline, 0, MAXLINE);
        sprintf(sendline, "GET /ping?ping_time=%ld HTTP/1.1\r\n", send_time);
        strcat(sendline, "Host: 172.31.10.131\r\n");
        strcat(sendline, "Content-Type: application/json\n");
        strcat(sendline, "\r\n");
        // printf("%s\n", sendline);
        n = write(sockfd, sendline, strlen(sendline));
        n = read(sockfd, recvline, MAXLINE);
        if (n == 0)
        {
            printf("ping client: server terminated prematurely\n");
            break;
        }

        long int now = now_timesatmp_us();
        record_lag(now - send_time);
        // printf("receive at %ld, content:%s, cost = %ld\n\n\n\n\n", now, recvline, now - send_time);
        // printf("\n\n");
        // 如果用标准库的缓存流输出有时会出现问题
        // fputs(recvline, stdout);
    }
}