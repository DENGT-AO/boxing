#ifndef _MEMORY_CHECK_H_
#define _MEMORY_CHECK_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

#define BUF_SIZE_1024        1024        
#define CMD_SIZE_1024        1024
#define NAME_SIZE_512       512                
#define RECV_NUM        64                              //接收拆分后的buf
#define M_1M            1024                            //单位均为KB
#define M_10M           1024 * 10       
#define M_50M           1024 * 50
#define M_100M          1024 * 100
#define M_500M          1024 * 500
#define MAX_LOG_SIZE_10M    1024 * 1024 * 10                //日志文件10M
#define THREAD_NUM      3
#define LOG_DIR         "/var/user/data/memroycheck"
#define LOG_FILE        "/var/user/data/memroycheck/memroy.log"


typedef void (*log_LockFn)(void *data, int lock);

typedef struct {
    char name[NAME_SIZE_512];
    int flag;
    long size;
    pid_t pid;
} Memory_Check;


struct log_conf {
    void *data;
    log_LockFn lock;                                    //文件单线程操作
    char file_name[RECV_NUM];                           // 文件名
    FILE *fp;                                           // 文件指针
    int max_size;                                       // 文件size最大值，超过范围回滚
};


extern int Log_Init();
extern int Check_Fork_Memroy();
extern int Check_Memroy_Log();


#endif