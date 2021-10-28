/*
 * $Id$ --
 *
 *   Mobius PYSDK Flag Ops
 *
 * Copyright (c) 2001-2012 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 09/11/2019
 * Author: zhengyb
 *
 */
#include <unistd.h>
#include <error.h>
#include "shared.h"
#include "mobius_pysdk_img.h"

static void mobius_pysdk_flag_main_help(void)
{
	printf("usage:\n");
	printf("mobius-pysdk-flag [Option]\n");
	printf("  Options:\n");	
	printf("    -h: display this message\n");
	printf("    -r: read the pysdk flag\n");
	printf("    -c: clear the pysdk flag (as S0: 0xFFFFFFFF)\n");
	printf("    -n: goto the next state\n");
	printf("    -s <state>: 0~3\n");
	printf("\n");
}

typedef enum{
	FLAG_CMD_READ,
	FLAG_CMD_CLEAR,
	FLAG_CMD_NEXT,
	FLAG_CMD_SET
}FLAG_CMD;

static int flag_state2byte(int state)
{
	int flag;

	switch(state){
		case 0: flag = PYSDK_IMG_FLAG_S0; break;
		case 1: flag = PYSDK_IMG_FLAG_S1; break;
		case 2: flag = PYSDK_IMG_FLAG_S2; break;
		case 3: flag = PYSDK_IMG_FLAG_S3; break;
		default: printf("Error flag.\n"); return -1;
	}

	return flag;
}

static int flag_byte2state(int byte)
{
	switch(byte){
		case PYSDK_IMG_FLAG_S0: return 0;
		case PYSDK_IMG_FLAG_S1: return 1;
		case PYSDK_IMG_FLAG_S2: return 2;
		case PYSDK_IMG_FLAG_S3: return 3;
		default: printf("bad byte 0x%02x\n", byte); return -1;
	}
}
int mobius_pysdk_flag_main(int argc, char *argv[])
{
	int c;
	FLAG_CMD cmd=FLAG_CMD_READ;
	int state=0;
	int flag;

	if (argc < 2) {
		mobius_pysdk_flag_main_help();
		return -1;
	}
		
	while ((c = getopt(argc, argv,"rhcns:")) != -1) {
		switch (c) {
		case 'h':
			mobius_pysdk_flag_main_help();
			return 0;
		case 'r':
			cmd = FLAG_CMD_READ;
			break;
		case 'c':
			cmd = FLAG_CMD_CLEAR;
			break;
		case 'n':
			cmd = FLAG_CMD_NEXT;
			break;
		case 's':		
			if (argc < 3) {
				mobius_pysdk_flag_main_help();
				return -1;
			}
			cmd = FLAG_CMD_SET;
			state = atoi(optarg);
			break;
		default:
			printf("ignore unknown arg: -%c %s\n", c, optarg ? optarg : "");
			return -1;
		}
	}
	switch(cmd){
	case FLAG_CMD_READ:
		flag = pysdk_img_flag();
		printf("Mobius Pysdk Flag: 0x%02X, state(%d)\n", flag, flag_byte2state(flag));
		break;
	case FLAG_CMD_CLEAR:
		erase_pysdk_img_flag();
		printf("Cleared.\n");
		break;
	case FLAG_CMD_SET:
		flag = flag_state2byte(state);
		erase_set_pysdk_img_flag(flag);
		printf("OK\n");
		break;
	case FLAG_CMD_NEXT:
		flag = pysdk_img_flag();
		switch(flag){
			case PYSDK_IMG_FLAG_S0: set_pysdk_img_flag(PYSDK_IMG_FLAG_S1); break;
			case PYSDK_IMG_FLAG_S1: set_pysdk_img_flag(PYSDK_IMG_FLAG_S2); break;
			case PYSDK_IMG_FLAG_S2: set_pysdk_img_flag(PYSDK_IMG_FLAG_S3); break;
			case PYSDK_IMG_FLAG_S3: erase_pysdk_img_flag(); break;
			default: printf("Error flag.\n"); return -1;
		}
		printf("OK\n");
		break;
	default:
		printf("Error command.\n");
		return -1;
	}
	return 0;
}