/*
 * libfdt - Flat Device Tree manipulation (build/run environment adaptation)
 * Copyright (C) 2007 Gerald Van Baren, Custom IDEAS, vanbaren@cideas.com
 * Original version written by David Gibson, IBM Corporation.
 *
 * SPDX-License-Identifier:	LGPL-2.1+
 */

#ifndef _LIBFDT_ENV_H
#define _LIBFDT_ENV_H

#include <linux/types.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <asm/byteorder.h>
#include <string.h>
#include <stdlib.h>

typedef __be16 fdt16_t;
typedef __be32 fdt32_t;
typedef __be64 fdt64_t;

#define fdt32_to_cpu(x)		__be32_to_cpu(x)
#define cpu_to_fdt32(x)		__cpu_to_be32(x)
#define fdt64_to_cpu(x)		__be64_to_cpu(x)
#define cpu_to_fdt64(x)		__cpu_to_be64(x)
#define cpu_to_uimage(x)  __cpu_to_be32(x)

#endif /* _LIBFDT_ENV_H */
