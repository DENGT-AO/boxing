#include <stdio.h>
#include <event2/event.h>
#include <stdlib.h>
#include <sys/time.h>


//编译方式：gcc -o timer timer.c -levent

struct event * time_event = NULL;
struct event_base *base = NULL;

void PrintTime()
{
    struct timeval timer;
    gettimeofday(&timer, NULL);
    volatile uint current_time = (uint)(timer.tv_sec);
    printf("current_time:%d\n", current_time);
}

void timeout_cb(evutil_socket_t fd, short event_flag, void *arg)
{
    PrintTime();
    printf("flag = %d \n", (int)event_flag);

    //重新设置timer为未决状态
    /*
    struct timeval timeout = {0, 0};
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    event_add(time_event, &timeout);
    */
}

int main(int argc, char **argv)
{

    struct timeval timeout = {0, 0};
    int ret = 0;
    struct event_base* base = NULL;


    //创建base结构体
    base = event_base_new();
    if (base == NULL)
    {
        printf("create event base error!\n");
        exit(1);
    }
    
    //设置超时事件
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    //EV_PERSIST可使事件持续为非未决状态
    time_event = event_new(base, -1, EV_TIMEOUT | EV_PERSIST, timeout_cb, (void *)&time_event);
    //time_event = event_new(base, -1, EV_PERSIST, cb_func, event_self_cbarg());  //持久超时
    //event_self_cbarg()将事件自己传入回调函数
    //time_event = evtimer_new(base, timeout_cb, &time_event);  //一次超时

    //添加事件为未决状态
    event_add(time_event, &timeout);
    event_base_dispatch(base);

    event_base_free(base);
    
    return 0;
}
