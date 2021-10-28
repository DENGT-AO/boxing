/*
 * $Id$ --
 *
 *   usbreset -- send a USB port reset to a USB device
 *
 * Copyright (c) 2001-2019 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 11/19/2019
 * Author: wucl
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <linux/usbdevice_fs.h>


#ifdef STANDALONE
int main(int argc, char *argv[])
#else
int usbreset_main(int argc, char *argv[]) 
#endif
{
    const char *filename = NULL;
    int fd;
    int rc;

    if (argc != 2) {
        fprintf(stderr, "Usage: usbreset /dev/bus/usb/<bus>/<dev>/\n");
        return 1;
    }
    filename = argv[1];

    fd = open(filename, O_WRONLY);
    if (fd < 0) {
        perror("Error opening output file");
        return 1;
    }

    printf("Resetting USB device %s\n", filename);
    rc = ioctl(fd, USBDEVFS_RESET, 0);
    if (rc < 0) {
        perror("Error in ioctl");
	close(fd);
        return 1;
    }
    printf("Reset successful\n");

    close(fd);
    return 0;
}
