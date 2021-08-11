#include <stdio.h>
#include <mosquitto.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


#define HOST "127.0.0.1"
#define PORT 1883
#define KEEP_ALIVE 60
#define MAXSIZE 512

// 定义运行标志决定是否需要结束
static int running = 1;

void my_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
    printf("Call the function: on_connect\n");

    if (rc)
    {
            // 连接错误，退出程序
            printf("on_connect error!\n");
            exit(1);
    }
    else
    {
        // 订阅主题
        // 参数：句柄、id、订阅的主题、qos
        if(mosquitto_subscribe(mosq, NULL, "a/b/c/#", 0)){
            printf("Set the topic error!\n");
            exit(1);
        }
    }
}
void my_subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
        printf("Call the function: on_subscribe\n");
}

//
void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    printf("Call the function: on_message\n");
    printf("Recieve a message of %s : %s\n", (char *)msg->topic, (char *)msg->payload);

    if(0 == strcmp(msg->payload, "quit\n"))
    {
        mosquitto_disconnect(mosq);
        running = 0;
    }
}

int main()
{
    struct mosquitto *mosq = NULL;
    int ret;
    char buf[MAXSIZE] = {'0'};

    //库初始化
    ret = mosquitto_lib_init();
    if (ret)
    {
        printf("init lib failed!\n");
        return -1;
    }

    //创建mosq实例
    mosq = mosquitto_new("test_sub", true, NULL);
    if (!mosq)
    {
        printf("mosq struct create failed! error num = %d\n", ret);
        ret = mosquitto_lib_cleanup();
        return -1;
    }

    //消息回调
    mosquitto_connect_callback_set(mosq, my_connect_callback);
    mosquitto_message_callback_set(mosq, my_message_callback);
    mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);

    //连接服务器
    ret = mosquitto_connect(mosq, HOST, PORT, KEEP_ALIVE);
    if (ret)
    {
        printf("client connect to broker failed!\n");
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return -1;
    }

    while(running)
    {
        //通道连通后，通过线程去处理通道中的网络消息,这里处理完后才会回调my_message_callback
        mosquitto_loop(mosq, -1, 1);
    }

    // 结束后的清理工作
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    printf("End!\n");

    return 0;
}