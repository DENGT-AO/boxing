#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <arpa/inet.h>

#include "server_info.h"

/**************************************************************** 
 usage example: 
          ./a.out --set-debug 1 --set-ip 9.9.9.9 --set-port 9999
*****************************************************************/

enum option_flag {
        SERVER_INFO_HELP = 0,
        SERVER_INFO_SET_IP,
        SERVER_INFO_SET_PORT,
        SERVER_INFO_SET_DEBUG
};

struct help_info {
        char *option_name;
        char *option_value;
        char *option_extra;
};

struct help_info options[] = {
        {"help", "", "server cmd help info"},
        {"set-ip", "A.B.C.D", "set server ip address"},
        {"set-port", "<1-65535>", "set server port"},
        {"set-debug", "0 or 1", "if debug value, if 1, log to file"},
        {NULL, NULL, NULL}
};

void server_cmd_help()
{
    struct help_info *opt = NULL;

    printf("a.out usage:\n");
    for (opt = options; opt->option_name; opt++) {
        printf("    --");
        printf("%-*s", 20, opt->option_name);
        printf("%-*s", 20, opt->option_value);
        printf("%-*s", 50, opt->option_extra);
        printf("\n");
    }
}

void server_show_config(server_addr_t *server_info)
{
    if (!server_info) {
        return;    
    }

#define SERVER_SHOW_CONFIG_LEN 40
    printf("%-*s0x%08x\n", SERVER_SHOW_CONFIG_LEN, "Server ip:", server_info->ip);
    printf("%-*s%d\n", SERVER_SHOW_CONFIG_LEN, "Server port:", server_info->port);
    printf("%-*s%d\n", SERVER_SHOW_CONFIG_LEN, "Server debug:", server_info->debug);
}

int set_server_ip(server_addr_t *server_info, char *ip)
{
    unsigned int value;

    if (!server_info || !ip) {
        return -1;
    }
    
    // return value: 1. success 0. invalid ip address -1. invalid address family
    if (inet_pton(AF_INET, ip, &value) <= 0) {
        return -1;
    }
    server_info->ip = ntohl(value);

    return 0;
}

int set_server_port(server_addr_t *server_info, char *port)
{
    // value should not be short int type, otherwise it will never biger than 65535
    int value;

    if (!server_info || !port) {
        return -1;
    }

    value = atoi(port);
    if (value < 1 || value > 65535) {
        return -1;
    }
    server_info->port = value;

    return 0;
}

int set_debug_status(server_addr_t *server_info, char *status)
{
    int value;

    if (!server_info || !status) {
        return -1;
    }

    value = atoi(status);
    if (value != 0 && value != 1) {
        return -1;
    }
    server_info->debug = value;

    return 0;
}

int main(int argc, char *argv[]) 
{
    int opt = -1;
    int index = -1;
    int retval = -1;
    int args_flag = -1;
    char *short_options = "";
    struct option long_options[] = {
        {"help", no_argument, &args_flag, SERVER_INFO_HELP},        
        {"set-ip", required_argument, &args_flag, SERVER_INFO_SET_IP},        
        {"set-port", required_argument, &args_flag, SERVER_INFO_SET_PORT},        
        {"set-debug", required_argument, &args_flag, SERVER_INFO_SET_DEBUG},        
        {NULL, 0, NULL, 0}        
    };

    server_addr_t serv;
    serv.debug = 0;
    serv.port = 6666;
    serv.ip = 0x06060606;

    if (argc < 2) {
        server_cmd_help();
    }

    while (-1 != (opt = getopt_long(argc, argv, short_options, long_options, &index))) {
        // if member flag of struct option is not NULL, it return 0 when success. 
        if (opt) {
            server_cmd_help();
            goto INVALID_PARAMETER;
        }
        switch (args_flag) {
            case SERVER_INFO_SET_IP:
                retval = set_server_ip(&serv, optarg);
                if (retval) {
                    goto INVALID_PARAMETER;
                }
                break;
            case SERVER_INFO_SET_PORT:
                retval = set_server_port(&serv, optarg);
                if (retval) {
                    goto INVALID_PARAMETER;
                }
                break;
            case SERVER_INFO_SET_DEBUG:
                retval = set_debug_status(&serv, optarg);
                if (retval) {
                    goto INVALID_PARAMETER;
                }
                break;
            case SERVER_INFO_HELP:
            default:
                server_cmd_help();
                break;
        }
    }

    if (serv.debug) {
        server_show_config(&serv);
    }

    return 0;

INVALID_PARAMETER:
    printf("invalid parameter, run a.out --help for usage information\n");
    return -1;
}
