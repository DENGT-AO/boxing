/*
 * $Id$ --
 *
 *   rtc cmd implementation
 *
 * Copyright (c) 2001-2010 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 06/13/2010
 * Author: Jianliang Zhang
 *
 */


#include <linux/rtc.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include "shared.h"

// -----------------------------------------------------------------------------
#ifdef STANDALONE
int main(int argc, char* argv[])
#else
int rtc_main(int argc, char* argv[])
#endif
{
	if(argc == 1) { 
		return saveRtc();
	} else if(strcmp(argv[1],"get") == 0) {
		time_t now;

		if (getRtc(&now) == 0) {			
			printf("%s\n",asctime(localtime(&now)));
		}

		return 0;
	} else if(strncmp(argv[1],"temperature",1) == 0) {
		float value;
		
		if (get_temperature(&value) != 0) {			
			printf("unable to get temperature\n");
		} else {
			printf("temperature:%.2f\n",value);
		}

		return 0;
	} 	 	

	return -EINVAL;
}
