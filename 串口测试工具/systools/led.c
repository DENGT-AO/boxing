/*
 * $Id$ --
 *
 *   GPIO tools
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
#include <stdio.h>
#include <string.h>
#include "ledcon.h"

#if 1
static void Usage(char *name)
{
	printf(	"Usage: %s on|off|flash|fflash warn|status|error|vpn|wlan|level0|level1|level2|tf|net\n"
		"              gpior reset|mpower|mrst|simswitch|phy1rst|phy2rst\n"
		"              gpiow mpower|mrst|simswitch|phy1rst|phy2rst high|low\n"
		"              \n", name);
}
#else
static void Usage(char *name)
{
	printf(	"Usage: %s on|off|flash|flash-slow usr1|usr2|usr3|usr4\n"
		"              \n", name);
}
#endif
/*
 *	main function to set leds' status.
 *	NERVER call this function, call setled() in your code instead!
 *		argv[1] : led command
 *		argv[2] : led id
 */
#ifdef STANDALONE
int main(int argc, char *argv[])
#else
int ledcon_main(int argc, char *argv[])
#endif
{
	int cmd;
	int led_gpio;
	int value;

	if (argc < 3) goto exit;
	if(strcasecmp(argv[1], "on")==0) cmd = LEDMAN_CMD_ON;
	else if(strcasecmp(argv[1], "off")==0) cmd = LEDMAN_CMD_OFF;
	else if(strcasecmp(argv[1], "flash")==0) cmd = LEDMAN_CMD_FLASH;
	else if(strcasecmp(argv[1], "fflash")==0) cmd = LEDMAN_CMD_FLASH_FAST;
	else if(strcasecmp(argv[1], "gpiow")==0) cmd = GPIO_WRITE;
	else if(strcasecmp(argv[1], "gpior")==0) cmd = GPIO_READ;
	else goto exit;	
	
	if(strcasecmp(argv[2], "status")==0) led_gpio = LED_STATUS;
	else if(strcasecmp(argv[2], "warn")==0) led_gpio = LED_WARN;
	else if(strcasecmp(argv[2], "error")==0) led_gpio = LED_ERROR;
	else if(strcasecmp(argv[2], "vpn")==0) led_gpio = LED_VPN;
	else if(strcasecmp(argv[2], "net")==0) led_gpio = LED_NET;
	else if(strcasecmp(argv[2], "wlan")==0) led_gpio = LED_WLAN;
	else if(strcasecmp(argv[2], "usr1")==0) led_gpio = LED_USR1;
	else if(strcasecmp(argv[2], "usr2")==0) led_gpio = LED_USR2;
	else if(strcasecmp(argv[2], "usr3")==0) led_gpio = LED_USR3;
	else if(strcasecmp(argv[2], "usr4")==0) led_gpio = LED_USR4;
	else if(strcasecmp(argv[2], "level0")==0) led_gpio = LED_LEVEL0;
	else if(strcasecmp(argv[2], "level1")==0) led_gpio = LED_LEVEL1;
	else if(strcasecmp(argv[2], "level2")==0) led_gpio = LED_LEVEL2;
#ifdef INHAND_IG9
	else if(strcasecmp(argv[2], "tf")==0) led_gpio = LED_TF;
#endif
	else if(strcasecmp(argv[2], "reset")==0) led_gpio = GPIO_RESET;
	else if(strcasecmp(argv[2], "mpower")==0) led_gpio = GPIO_MPOWER;
	else if(strcasecmp(argv[2], "mrst")==0) led_gpio = GPIO_MRST;
	else if(strcasecmp(argv[2], "simswitch")==0) led_gpio = GPIO_SIM_SWITCH;
	else if(strcasecmp(argv[2], "phy1rst")==0) led_gpio = GPIO_PHY1RST;
	else if(strcasecmp(argv[2], "phy2rst")==0) led_gpio = GPIO_PHY2RST;
	else goto exit;

	if(cmd == GPIO_READ){
		//printf("gpio:%d cmd:%d\n", led_gpio, cmd);
		gpio(cmd,led_gpio,&value);
		printf("Read %d from %d\n", value, led_gpio);
		return 0;
	}else if(cmd == GPIO_WRITE){
		if(argc<=3) goto exit;
		
		if(strcasecmp(argv[3], "high")==0) value = 1;
		else if(strcasecmp(argv[3], "low")==0) value = 0;
		return gpio(cmd,led_gpio,&value);
	}

	return ledcon(cmd, led_gpio);
exit:
	Usage(argv[0]);
	return 0;
}

