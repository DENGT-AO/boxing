
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mosquitto.h>
 
#define HOST "localhost"
#define PORT  1883
#define KEEP_ALIVE 60
#define MSG_MAX_SIZE  512
 
static int running = 1;
 
void my_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
    printf("Call the function: my_connect_callback\n");
 
}
 
void my_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
    printf("Call the function: my_disconnect_callback\n");
    running = 0;
}
 
void my_publish_callback(struct mosquitto *mosq, void *obj, int mid)
{
    printf("Call the function: my_publish_callback\n");
 
}
 
 
int main()
{
    int ret;
    struct mosquitto *mosq;
    char buff[MSG_MAX_SIZE];

    //初始化libmosquitto库
    ret = mosquitto_lib_init();
    if(ret)
    {
        printf("Init lib error!\n");
        return -1;
    }
    //创建一个发布端实例
    mosq =  mosquitto_new("pub_test", true, NULL);
    if(mosq == NULL)
    {
        printf("New pub_test error!\n");
        mosquitto_lib_cleanup();
        return -1;
    }
    
    //设置回调函数
    mosquitto_connect_callback_set(mosq, my_connect_callback);
    mosquitto_disconnect_callback_set(mosq, my_disconnect_callback);
    mosquitto_publish_callback_set(mosq, my_publish_callback);

    // 连接至服务器
    // 参数：句柄、ip（host）、端口、心跳
    ret = mosquitto_connect(mosq, HOST, PORT, KEEP_ALIVE);
    if(ret)
    {
        printf("Connect server error!\n");
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return -1;
    }

    printf("Start!\n");
    //mosquitto_loop_start作用是开启一个线程，在线程里不停的调用 mosquitto_loop() 来处理网络信息             
    int loop = mosquitto_loop_start(mosq); 
    if(loop != MOSQ_ERR_SUCCESS)
    {
        printf("mosquitto loop error\n");
        return 1;
    }

    int a = 10;

    while(fgets(buff, MSG_MAX_SIZE, stdin) != NULL)
    {
        /*发布消息*/
        mosquitto_publish(mosq,&a,"topic1",strlen(buff)+1,buff,1,0);
        sleep(1);
        if(0 == strcmp(buff, "quit\n"))
        {
            mosquitto_disconnect(mosq);
            break;
        }
        memset(buff,0,sizeof(buff));
    }

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    printf("End!\n");

    return 0;
}