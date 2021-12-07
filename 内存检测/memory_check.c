#include "memory_check.h"


Memory_Check g_forkinfo[] = {
    {
        .name = "init",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "syswatcher",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "watchdog -t 5 /dev/watchdog",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "syslogd -S -n -m 0 -L -s 5120 -F 8 -z 256",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "admind",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "alarmd",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "netevtd",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "interface",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "redial",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "dot11d",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "bridged",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "xdsl",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "ipsecwatcher2",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "vpnd",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "agent",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "emaild",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "sntpc",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "gre_agent",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "ih_route",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "firewall",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "sla",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "trackd",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "backupd",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "ih_mrouted",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "certmanager",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "termon",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "gpsd",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "crond",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "dtu",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "python_agent",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "trafficd",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "zebra",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "ripd",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "ospfd",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "bgpd",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "mosquitto",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "httpd",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "telnetd",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "htpdmanager",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "Erlang",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "nginx: master process",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "DeviceManager",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "dropbear",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "dockerd",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "InConnect",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "portainer",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "dnsmasq",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "nginx: worker process",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "docker-containerd",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "supervisord",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "api_gateway",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "klogd",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "redial",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "openvpn",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    //app相关进程
    {
        .name = "user/bin/device_supervisor",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "user/app/device_supervisor/DataHub",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "user/app/device_supervisor/src/qckfs.pyc",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "user/app/device_supervisor/ModbusDriver",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "user/app/device_supervisor/MqttAgent",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },
    {
        .name = "user/app/device_supervisor/publish/HslTechnology.EdgeServer",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    }
    /*
    //所有测点控制器   有多少协议  就有多少进程  测点占用内存无法估量~暂不监控
    {
        .name = "user/app/device_supervisor/src/drvr.pyc",
        .flag = -1,
        .size = 0,
        .pid  = -1,
    },*/
};


struct log_conf g_log;

static int Log_File(char *buf)
{
    int ret = 0;
    FILE *fp = NULL;
    struct stat st_info;

    fp = fopen(LOG_FILE, "a+");
    if (NULL == fp) {
        printf("fopen() failed.\n");
        return -1;
    }

    stat(g_log.file_name, &st_info);        //单位为字节
    if (st_info.st_size > g_log.max_size) {
        truncate(g_log.file_name, 0);
        fseek(fp, 0, SEEK_SET);
    }

    if (!fwrite(buf, strlen(buf), 1, fp)) {
        printf("Failed to write file: %s(%d:%s)\n", fp, errno, strerror(errno));
        fclose(fp);
        fp = NULL;
		return -1;
	}

    fclose(fp);
    fp = NULL;

    return 0;
}

static int Memroy_Check(Memory_Check *forkinfo, char **buf, char *tmp_buf)
{
    long size = 0;
    char log[BUF_SIZE_1024] = {0};
    int ret = 0;

    //printf("Memroy_Check in\n");
    //printf("strstr");

    tmp_buf = strstr(buf[2], "m");
    if (tmp_buf) {
        *tmp_buf = '\0';       //将m删除，取出数字
        size = atoi(buf[2]) * M_1M;
    } else {
        size = atoi(buf[2]);
    }
    //printf("check");
    //printf("size = %ld, initial size = %ld\n", size, forkinfo->size);
    if (size > M_500M && (size > forkinfo->size) && ((size - forkinfo->size) > 0)) { //M_50M
        //写文件操作  写入进程号，进程信息，起始size，当前size
        snprintf(log, sizeof(log), \
            "%s has a memory leak, PID: %d, Initial memory size: %d KB, Current memory size: %d KB. Memory leak Has exceeded 50M.\n", \
            forkinfo->name, forkinfo->pid, forkinfo->size, size);
        printf("%s\n", log);
    } else if (size > forkinfo->size && (size - forkinfo->size) > 0) {              //M_10M
        //写文件操作  写入进程号，进程信息，起始size，当前size
        snprintf(log, sizeof(log), \
            "%s has a memory leak, PID: %d, Initial memory size: %d KB, Current memory size: %d KB. Memory leak Has exceeded 10M.\n", \
            forkinfo->name, forkinfo->pid, forkinfo->size, size);
        printf("%s\n", log);
    }
    

    if(strlen(log) > 0 && Log_File(log)) {
        printf("Failed to write log file: %s(%d:%s)\n", g_log.file_name, errno, strerror(errno));
        return -1;
    }

    //printf("Memroy_Check out\n");
    return 0;
}

static int Split(char *src, const char *separator, char **dest, int *num)
{
	char *pNext = NULL;
	int count = 0;

    //printf("Split in\n");

	if (src == NULL || strlen(src) == 0) {
		return -1;
	}
		
	if (separator == NULL || strlen(separator) == 0) {
		return -1;
	}
		
	pNext = strtok(src, separator);
	while (pNext != NULL) {
		*dest++ = pNext;
		++count;
		pNext = strtok(NULL, separator);
	}
	*num = count;


    //printf("Split out\n");
    return 0;
}

static int Check_Fork_Memroy()
{
    int i     = 0;
    int j     = 0;
    int ret   = 0;
    int num   = 0;
    char cmd[BUF_SIZE_1024]      = {"\0"};
    char buf[BUF_SIZE_1024]      = {"\0"};
    char *revbuf[RECV_NUM]       = {'\0'};
    char *tmp_recv = NULL;
    FILE *fp = NULL;

    //printf("Check_Fork_Memroy in\n");


    //另一种方案是cat /proc/xxx进程号/status | grep VmSize
    for (i = 0; i < sizeof(g_forkinfo)/sizeof(g_forkinfo[0]); i++) {
        //printf("eee");
        snprintf(cmd, sizeof(cmd), "ps | grep -w '%s' | grep -v grep", g_forkinfo[i].name);
        //printf("fff");
        //printf("cmd =  %s\n", cmd);
		fp = popen(cmd, "r");
		if (!fp) {
			printf("failed to open 'popen', errno = %d(%s)\n",errno, strerror(errno));
			goto ERR;
		}
        //printf("ggg");
		fgets(buf, sizeof(buf), fp);
        //printf("hh");
        if (strlen(buf) > 0) {
            //printf("aa");
            //printf("\ncount = %d, %s\n", i, buf);
            ret = Split(buf, " ", revbuf, &num);
            if (ret) {
                printf("failed to split string, errno = %d(%s)\n",errno, strerror(errno));
                goto ERR;
            }

            //flag为-1或者pid不等表示程序新启动，反之则表示需要更新size
            if (g_forkinfo[i].flag == -1 || g_forkinfo[i].pid != atoi(revbuf[0])) {
                //printf("bb");
                g_forkinfo[i].pid = atoi(revbuf[0]);
                tmp_recv = strstr(revbuf[2], "m");                           //单位为byte
                if (tmp_recv) {
                    *tmp_recv = '\0';       //将m删除，取出数字
                    g_forkinfo[i].size = atoi(revbuf[2]) * 1024;
                } else {
                    g_forkinfo[i].size = atoi(revbuf[2]);
                }
                
                g_forkinfo[i].flag = 1;
            } else {
                //printf("cc");
                ret = Memroy_Check(&g_forkinfo[i], revbuf, tmp_recv);
                if (ret) {
                    printf("failed to memroy check, errno = %d(%s)\n",errno, strerror(errno));
                    goto ERR;
                }
            }
        } else {
            //未启动就不管
            printf("g_forkinfo[i].name = %s has not startup", g_forkinfo[i].name);
        }

        if (fp)
        {
            pclose(fp);
            fp = NULL;
        }
    }

    //printf("Check_Fork_Memroy out\n");
    return 0;
ERR:
    if (fp)
    {
        pclose(fp);
        fp = NULL;
    }

    return -1;
}

void log_lock(void *data, int status)
{
    if (NULL == data) 
        return;

    if (status) {
        pthread_mutex_lock((pthread_mutex_t *)data);
    } else {
        pthread_mutex_unlock((pthread_mutex_t *)data);
    }
}


static int Log_Init()
{
    unsigned long long p_arg[THREAD_NUM];
    /*
        pthread_t p_id[THREAD_NUM];
        pthread_mutex_t log_mutex;
    */
    char cmd[BUF_SIZE_1024] = {0};
    int ret = 0;
    FILE *fp = NULL;

    //pthread_mutex_init(&log_mutex, NULL);         使用的时候再用

    if (access(LOG_FILE, F_OK) == -1) {
        snprintf(cmd, sizeof(cmd), "mkdir -p %s; touch %s;", LOG_DIR, LOG_FILE);
        ret = system(cmd);
        if(ret == -1 || (!WIFEXITED(ret)) || (WEXITSTATUS(ret) != 0)) {
            printf("mkdir %s failed(%d:%d)", LOG_DIR, WIFEXITED(ret), WEXITSTATUS(ret));
            goto ERR;
        }
    }

    
    memset(&g_log, 0, sizeof(struct log_conf));
    //g_log.data         = &log_mutex;
    g_log.lock         = log_lock;
    //g_log.fp           = fp;
    g_log.max_size     = 1024;
    //g_log.max_size     = MAX_LOG_SIZE_10M;
    strncpy(g_log.file_name, LOG_FILE, sizeof(g_log.file_name) - 1);

    //文件释放在出错时才操作。正常情况不会停止
    return 0;

ERR:
    if (fp)
    {
        fclose(fp);
        fp = NULL;
    }
    
    return -1;
}

static void *Check_Log_Thread(void *arg)
{
    char *cmd[CMD_SIZE_1024] = {0};
    char *buf[CMD_SIZE_1024] = {0};
    FILE *fp = NULL;
    int size = 0;

    /*
    snprintf(cmd, sizeof(cmd), "ls -al %s | awk -F ' ' '{print $5}'", LOG_FILE);
    fp = popen(cmd, "r");
    if (!fp) {
        printf("failed to open 'popen', errno = %d(%s)\n", errno, strerror(errno));
        goto ERR;
    }

    fgets(buf, sizeof(buf), fp);
    if (strlen(buf) > 0) {

    }*/
    //printf("Check_Log_Thread in\n");

    if (access(LOG_FILE, F_OK) == 0) {
        fp = fopen(LOG_FILE, "r");
        if(!fp) {
            printf("failed to open 'popen', errno = %d(%s)\n", errno, strerror(errno));
            return NULL;
        }

        fseek(fp, 0L, SEEK_END); 
        size = ftell(fp);       //单位为字节
        
        if (size > 0) {
            //待适配系统日志
            printf("The system has memroy leak, please check the %s to get details.\n", LOG_FILE);
        }
        fclose(fp);
    }

    //printf("Check_Log_Thread out\n");
    return NULL;
}

static int Check_Memroy_Log()
{
    pthread_mutex_t log_mutex;
    pthread_t p_id = -1;
    int ret = 0;

    pthread_mutex_init(&log_mutex, NULL);

    ret = pthread_create(&p_id, NULL, Check_Log_Thread, (void *)&p_id);
    if (ret) {
        printf("Failed to create Check_Log_Thread. errno=%d(%s)\n", errno, strerror(errno));
        goto ERR;
    }

    if (pthread_join(p_id, NULL )) {
        printf("error join thread. errno=%d(%s)\n", errno, strerror(errno));
        goto ERR;
    }
    pthread_mutex_destroy(&log_mutex);
    return 0;

ERR:
    pthread_mutex_destroy(&log_mutex);
    return -1;
}

int main(int argc, char **argv)
{
    int ret = 0;
    int i= 0;

    ret = Log_Init();
    if (ret) {
        printf("failed to log init, errno = %d(%s)\n",errno, strerror(errno));
        return -1;
    }

    while (1) {
        ret = Check_Fork_Memroy();
        if (ret) {
            printf("failed to check fork memroy, errno = %d(%s)\n",errno, strerror(errno));
            return -1;
        }

        ret = Check_Memroy_Log();
        if (ret) {
            printf("failed to check memroy log file, errno = %d(%s)\n",errno, strerror(errno));
            return -1;
        }
    }

    if (g_log.fp)
    {
        fclose(g_log.fp);
        g_log.fp = NULL;
    }

    return 0;
}