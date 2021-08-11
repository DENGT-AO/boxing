#include <stdio.h>
#include <mosquitto.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define HOST "127.0.0.1"
#define PORT 1883
#define KEEP_ALIVE 60
#define MAXSIZE 512

int main()
{
    int ret = 0;
    struct mosquitto *mosq = NULL;
    char buf[MAXSIZE] = {'0'};
    
    //库初始化
    ret = mosquitto_lib_init();
    if (ret)
    {
        printf("mosquitto lib init error!\n");
    }

    //新建mosq实例
    mosq = mosquitto_new("test_pub", true, NULL);
    if (!mosq)
    {
        printf("mosq struct create failed! error num = %d\n", ret);
        ret = mosquitto_lib_cleanup();
        return -1;
    }

    //连接broker
    ret = mosquitto_connect(mosq, HOST, PORT, KEEP_ALIVE);
    if (ret)
    {
        printf("connect to broker failed,errornum = %d!\n", ret);
        mosquitto_destroy(mosq);
        ret = mosquitto_lib_cleanup();
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

    while (fgets(buf, sizeof(buf), stdin) != NULL)
    {   
        //发布消息
        mosquitto_publish(mosq, NULL, "a/b/c/1", sizeof(buf), buf, 0, false);
        mosquitto_publish(mosq, NULL, "a/b/c/3", sizeof(buf), buf, 0, false);
        memset(buf, 0, sizeof(buf));
    }


    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}