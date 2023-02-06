#ifndef __COMMON__
#define __COMMON__

#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

long int now_timesatmp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

long int now_timesatmp_ms()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

long int now_timesatmp_us()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

int reach_send_time()
{
    static long int last = 0;

    if (last == 0)
    {
        last = now_timesatmp_us();
        return 1;
    }
    if (now_timesatmp_us() > last + 100000)
    {
        last = now_timesatmp_us();
        // printf("reach send time last time = %ld\n", last);
        return 1;
    }
    // printf("not reach send time last = %ld,now = %ld\n", last, now_timesatmp_us());
    return 0;
}

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

#endif