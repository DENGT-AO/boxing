/*
 * $Id$ --
 *
 *   InHand shell applets
 *
 * Copyright (c) 2001-2010 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 06/04/2010
 * Author: Jianliang Zhang
 *
 */

#include <syslog.h>
#include <string.h>
#include "ih_types.h"
#include "ih_ipc.h"
#include "license.h"
#include "sw_ipc.h"
#include "shared.h"
#include "bootenv.h"
#include "errno.h"
#ifdef INHAND_IDTU9
#include "rand_test.h"
#endif

extern int dnsc_main(int argc,char *argv[]);
extern int rtc_main(int argc, char* argv[]);
extern int reboothalt_main(int argc, char *argv[]);
extern int intools_main (int argc, char **argv);
extern int ledcon_main(int argc, char *argv[]);
extern int mtd_main(int argc, char *argv[]);
extern int iodebug_main(int argc, char *argv[]);
extern int modemsignal_main(int argc, char *argv[]);
extern int usbreset_main(int argc, char *argv[]);
extern int bootenv_main(int argc, char *argv[]);
extern int iscfg_main(int argc, char *argv[]);
extern int tipc_config_main(int argc, char *argv[]);
extern int sdebug_main(int argc, char* argv[]);
extern int stestserver_main(int argc, char* argv[]);
extern int comredirect_main(int argc, char* argv[]);
extern int iburn_main(int argc, char* argv[]);
extern int inpython_main(int argc, char* argv[]);
extern int bspatch_main(int argc, char* argv[]);
extern int devmem2_main(int argc, char* argv[]);
#if (!defined OPENDEVICE_OLD_PYSDK) && (!defined MOBIUS_PYTHON) 
extern int pysdk_install_main(int argc, char* argv[]);
extern int pyapp_install_main(int argc, char* argv[]);
extern int pyapp_uninstall_main(int argc, char* argv[]);
#endif
#ifdef MOBIUS_PYTHON
extern int mobius_pysdk_flag_main(int argc, char *argv[]);
extern int mobius_pysdk_install_main(int argc, char *argv[]);
extern int mobius_pyapp_install_main(int argc, char *argv[]);
#endif

#ifdef INHAND_IDTU9
int get_rand_main(int argc, char* argv[]);
#endif

// -----------------------------------------------------------------------------
typedef struct {
	const char *name;
	int (*main)(int argc, char *argv[]);
} applets_t;

static const applets_t applets[] = {
//	{ "halt",				reboothalt_main		},
	{ "reboot",				reboothalt_main		},
	{ "ledcon",				ledcon_main			},
	{ "rtc",				rtc_main			},
	{ "sdebug",				sdebug_main			},
	{ "stestserver",	    stestserver_main    },
	{ "comredirect",			comredirect_main			},
	{ "iodebug",			iodebug_main		},
	{ "modemsignal",			modemsignal_main		},
	{ "usbreset",			usbreset_main		},
	{ "mtd-write",			mtd_main			},
	{ "mtd-read",			mtd_main			},
	{ "mtd-erase",			mtd_main			},
	{ "dnsc",				dnsc_main			},	
	{ "bootenv",			bootenv_main		},
	{ "iscfg",				iscfg_main			},
	{ "intools",			intools_main		},
	{ "tipc-config",		tipc_config_main	},
	{ "iburn",			iburn_main			},
#ifndef MOBIUS_PYTHON	
	{ "inpython",		inpython_main		},
#endif
#ifdef INHAND_IDTU9
	{ "get_rand",		get_rand_main		},
	{ "rand",			get_rand_main		},
	{ "bspatch",		bspatch_main		},
#endif
	{ "devmem2",		devmem2_main		},
#if (!defined OPENDEVICE_OLD_PYSDK) && (!defined MOBIUS_PYTHON)
	{ "pysdk-install",		pysdk_install_main		},
	{ "pyapp-install",		pyapp_install_main		},
	{ "pyapp-uninstall",		pyapp_uninstall_main		},
#endif
#ifdef MOBIUS_PYTHON
	{ "mobius-pysdk-flag", mobius_pysdk_flag_main},
	{ "mobius-pysdk-install", mobius_pysdk_install_main},	
	{ "mobius-pyapp-install", mobius_pyapp_install_main},
#endif
	{NULL, NULL}
};

#ifdef INHAND_IDTU9
//////////////////////////////////////////////////////////////////////////
//	for geting rand file
//////////////////////////////////////////////////////////////////////////
int get_rand_main(int argc, char* argv[])
{
	int num = 1;
	int min = 0, max = 128;
	int length = 128*1024;
	char *server = NULL;
	char file[32];
	char buf[128];
	int i;
	int failed_times = 0;

	if(argc>1) length = atoi(argv[1]);

	if(argc>2) min = atoi(argv[2]);
	if(argc>3) max = atoi(argv[3]);
	if(argc>4) server = argv[4];

	num = max - min + 1;

	printf("get %d random, %d byte for each file, server is %s\n", num, length, server);
	chdir("/tmp");

	if (!strcmp(argv[0], "rand")) {
		rand_startup_test();
		//rand_cycle_test();
		//rand_one_time_test(128);
		//rand_one_time_test(10000);
	} else {
		//get rand

		for (i = min; i <= max; i++) {
			sprintf(file, "/tmp/random%d", i);
			if(!get_rand(file, length)) {
				printf("get random %d failed [%ld]\n", i, time(NULL)); 
				continue;
			}

			if(server) {
				snprintf(buf, sizeof(buf), "tftp -pr %s %s", file, server);
				system(buf);
				unlink(file);
			}
		}

		printf("get random %d/%d failed [%ld]\n",failed_times , num, time(NULL)); 
	}

	return 0;
}
#endif
	

static void  my_sysinfo_init (void) 
{
	char tmp[64];

	if (bootenv_get("version",tmp,sizeof(tmp))) {
		strlcpy(g_sysinfo.bootloader,tmp,MAX_BOOTLOADER);
	}
	
	if (bootenv_get("oem_name",tmp,sizeof(tmp))) {
		strlcpy(g_sysinfo.oem_name,tmp,MAX_OEM_NAME);
	}

	if (bootenv_get("productnumber",tmp,sizeof(tmp))) {
		strlcpy(g_sysinfo.product_number,tmp,MAX_PRODUCT_NUMBER);
	}

	if (bootenv_get("serialnumber",tmp,sizeof(tmp))) {
		strlcpy(g_sysinfo.serial_number,tmp,MAX_SERIAL_NUMBER);
	}

	if (bootenv_get("description",tmp,sizeof(tmp))) {
		strlcpy(g_sysinfo.description,tmp,MAX_DESCRIPTION);
	}

	if (bootenv_get("model_name",tmp,sizeof(tmp))) {
		strlcpy(g_sysinfo.model_name,tmp,MAX_MODEL_NAME);
	}

	if (bootenv_get("family_name",tmp,sizeof(tmp))) {
		strlcpy(g_sysinfo.family_name,tmp,MAX_FAMILY_NAME);
	}
}

int main(int argc, char **argv)
{
	char *base;
	const applets_t *a;
	int ret = 0;

	openlog("systools", LOG_PID, LOG_USER);

	my_sysinfo_init();
	ih_license_init(g_sysinfo.family_name, g_sysinfo.model_name, g_sysinfo.product_number);	

	base = strrchr(argv[0], '/');
	base = base ? base + 1 : argv[0];
	argv[0] = base;

	for (a = applets; a->name; ++a) {
		if (strcmp(base, a->name) == 0) {
			ret = a->main(argc, argv);
			break;
		}
	}

	if(!(a->name)) syslog(LOG_WARNING,"Unknown applet: %s\n", base);

	closelog();
	return ret;
}

