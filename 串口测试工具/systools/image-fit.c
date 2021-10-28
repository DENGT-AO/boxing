/*
 * Copyright (c) 2013, Google Inc.
 *
 * (C) Copyright 2008 Semihalf
 *
 * (C) Copyright 2000-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <asm/types.h>
#include <errno.h>
#include <sys/mman.h>
#include <syslog.h>

#include "image.h"
#include "openssl/md5.h"
//#include "sha1.h"
//#include "sha256.h"
#include "openssl/sha.h"
#include "shared.h"
#include "libfdt.h"

/**
 * fit_get_name - get FIT node name
 * @fit: pointer to the FIT format image header
 *
 * returns:
 *     NULL, on error
 *     pointer to node name, on success
 */
static inline const char *fit_get_name(const void *fit_hdr, int noffset, int *len)
{
	return fdt_get_name(fit_hdr, noffset, len);
}

static void fit_get_debug(const void *fit, int noffset,
		char *prop_name, int err)
{
	syslog(LOG_INFO, "Can't get '%s' property from FIT 0x%08lx, node: offset %d, name %s (%s)\n",
	      prop_name, (ulong)fit, noffset, fit_get_name(fit, noffset, NULL),
	      fdt_strerror(err));
}

/**
 * fit_image_get_data - get data property and its size for a given component image node
 * @fit: pointer to the FIT format image header
 * @noffset: component image node offset
 * @data: double pointer to void, will hold data property's data address
 * @size: pointer to size_t, will hold data property's data size
 *
 * fit_image_get_data() finds data property in a given component image node.
 * If the property is found its data start address and size are returned to
 * the caller.
 *
 * returns:
 *     0, on success
 *     -1, on failure
 */
int fit_image_get_data(const void *fit, int noffset,
		const void **data, size_t *size)
{
	int len;

	*data = fdt_getprop(fit, noffset, FIT_DATA_PROP, &len);
	if (*data == NULL) {
		fit_get_debug(fit, noffset, FIT_DATA_PROP, len);
		*size = 0;
		return -1;
	}

	*size = len;
	return 0;
}

/**
 * fit_image_hash_get_algo - get hash algorithm name
 * @fit: pointer to the FIT format image header
 * @noffset: hash node offset
 * @algo: double pointer to char, will hold pointer to the algorithm name
 *
 * fit_image_hash_get_algo() finds hash algorithm property in a given hash node.
 * If the property is found its data start address is returned to the caller.
 *
 * returns:
 *     0, on success
 *     -1, on failure
 */
int fit_image_hash_get_algo(const void *fit, int noffset, char **algo)
{
	int len;

	*algo = (char *)fdt_getprop(fit, noffset, FIT_ALGO_PROP, &len);
	if (*algo == NULL) {
		fit_get_debug(fit, noffset, FIT_ALGO_PROP, len);
		return -1;
	}

	return 0;
}

/**
 * fit_image_hash_get_value - get hash value and length
 * @fit: pointer to the FIT format image header
 * @noffset: hash node offset
 * @value: double pointer to uint8_t, will hold address of a hash value data
 * @value_len: pointer to an int, will hold hash data length
 *
 * fit_image_hash_get_value() finds hash value property in a given hash node.
 * If the property is found its data start address and size are returned to
 * the caller.
 *
 * returns:
 *     0, on success
 *     -1, on failure
 */
int fit_image_hash_get_value(const void *fit, int noffset, uint8_t **value,
				int *value_len)
{
	int len;

	*value = (uint8_t *)fdt_getprop(fit, noffset, FIT_VALUE_PROP, &len);
	if (*value == NULL) {
		fit_get_debug(fit, noffset, FIT_VALUE_PROP, len);
		*value_len = 0;
		return -1;
	}

	*value_len = len;
	return 0;
}

/**
 * fit_image_hash_get_ignore - get hash ignore flag
 * @fit: pointer to the FIT format image header
 * @noffset: hash node offset
 * @ignore: pointer to an int, will hold hash ignore flag
 *
 * fit_image_hash_get_ignore() finds hash ignore property in a given hash node.
 * If the property is found and non-zero, the hash algorithm is not verified by
 * u-boot automatically.
 *
 * returns:
 *     0, on ignore not found
 *     value, on ignore found
 */
static int fit_image_hash_get_ignore(const void *fit, int noffset, int *ignore)
{
	int len;
	int *value;

	value = (int *)fdt_getprop(fit, noffset, FIT_IGNORE_PROP, &len);
	if (value == NULL || len != sizeof(int))
		*ignore = 0;
	else
		*ignore = *value;

	return 0;
}

static inline phys_addr_t map_to_sysmem(const void *ptr)
{
	return (phys_addr_t)(uintptr_t)ptr;
}

ulong fit_get_end(const void *fit)
{
	return map_to_sysmem((void *)(fit + fdt_totalsize(fit)));
}

/**
 * fit_set_timestamp - set node timestamp property
 * @fit: pointer to the FIT format image header
 * @noffset: node offset
 * @timestamp: timestamp value to be set
 *
 * fit_set_timestamp() attempts to set timestamp property in the requested
 * node and returns operation status to the caller.
 *
 * returns:
 *     0, on success
 *     -ENOSPC if no space in device tree, -1 for other error
 */
int fit_set_timestamp(void *fit, int noffset, time_t timestamp)
{
	uint32_t t;
	int ret;

	t = cpu_to_uimage(timestamp);
	ret = fdt_setprop(fit, noffset, FIT_TIMESTAMP_PROP, &t,
				sizeof(uint32_t));
	if (ret) {
		syslog(LOG_INFO, "Can't set '%s' property for '%s' node (%s)\n",
		      FIT_TIMESTAMP_PROP, fit_get_name(fit, noffset, NULL),
		      fdt_strerror(ret));
		return ret == -FDT_ERR_NOSPACE ? -ENOSPC : -1;
	}

	return 0;
}

/**
 * calculate_hash - calculate and return hash for provided input data
 * @data: pointer to the input data
 * @data_len: data length
 * @algo: requested hash algorithm
 * @value: pointer to the char, will hold hash value data (caller must
 * allocate enough free space)
 * value_len: length of the calculated hash
 *
 * calculate_hash() computes input data hash according to the requested
 * algorithm.
 * Resulting hash value is placed in caller provided 'value' buffer, length
 * of the calculated hash is returned via value_len pointer argument.
 *
 * returns:
 *     0, on success
 *    -1, when algo is unsupported
 */
int calculate_hash(const void *data, int data_len, const char *algo,
			uint8_t *value, int *value_len)
{
	if (IMAGE_ENABLE_CRC32 && strcmp(algo, "crc32") == 0) {
		*((uint32_t *)value) = crc32(0, (char *)data, data_len);
		*((uint32_t *)value) = cpu_to_uimage(*((uint32_t *)value));
		*value_len = 4;
	} else if (IMAGE_ENABLE_SHA1 && strcmp(algo, "sha1") == 0) {
		SHA1((unsigned char *)data, data_len, (unsigned char *)value);
		*value_len = 20;
	} else if (IMAGE_ENABLE_SHA256 && strcmp(algo, "sha256") == 0) {
		SHA256((unsigned char *)data, data_len, (unsigned char *)value);
		*value_len = FIT_MAX_HASH_LEN;
	} else if (IMAGE_ENABLE_MD5 && strcmp(algo, "md5") == 0) {
		MD5((unsigned char *)data, data_len, (unsigned char *)value);
		*value_len = 16;
	} else {
		syslog(LOG_INFO, "Unsupported hash alogrithm\n");
		return -1;
	}

	return 0;
}

static int fit_image_check_hash(const void *fit, int noffset, const void *data,
				size_t size, char **err_msgp)
{
	uint8_t value[FIT_MAX_HASH_LEN];
	int value_len;
	char *algo;
	uint8_t *fit_value;
	int fit_value_len;
	int ignore;
	char buf[256], tmp[256];
	int i = 0;

	*err_msgp = NULL;

	if (fit_image_hash_get_algo(fit, noffset, &algo)) {
		*err_msgp = "Can't get hash algo property";
		return -1;
	}

	if (IMAGE_ENABLE_IGNORE) {
		fit_image_hash_get_ignore(fit, noffset, &ignore);
		if (ignore) {
			syslog(LOG_INFO, "-skipped ");
			return 0;
		}
	}

	if (fit_image_hash_get_value(fit, noffset, &fit_value, &fit_value_len)) {
		*err_msgp = "Can't get hash value property";
		return -1;
	}
	
	bzero(buf, sizeof(buf));
	for(i = 0; i < fit_value_len; i++){
		snprintf(buf + strlen(buf), sizeof(buf) -strlen(buf), "%2x", fit_value[i]);
	}

	if (calculate_hash(data, size, algo, value, &value_len)) {
		*err_msgp = "Unsupported hash algorithm";
		return -1;
	}

	bzero(tmp, sizeof(tmp));
	for(i = 0; i < value_len; i++){
		snprintf(tmp + strlen(tmp), sizeof(tmp) - strlen(tmp), "%2x", value[i]);
	}

	//syslog(LOG_INFO, "%s: 0x%s, caculate value: 0x%s", algo, buf, tmp);

	if (value_len != fit_value_len) {
		*err_msgp = "Bad hash value len";
		return -1;
	} else if (memcmp(value, fit_value, value_len) != 0) {
		*err_msgp = "Bad hash value";
		return -1;
	}

	return 0;
}

/**
 * fit_image_verify - verify data integrity
 * @fit: pointer to the FIT format image header
 * @image_noffset: component image node offset
 *
 * fit_image_verify() goes over component image hash nodes,
 * re-calculates each data hash and compares with the value stored in hash
 * node.
 *
 * returns:
 *     1, if all hashes are valid
 *     0, otherwise (or on error)
 */
int fit_image_verify(const void *fit, int image_noffset)
{
	const void	*data;
	size_t		size;
	int		noffset = 0;
	char		*err_msg = "";

	/* Get image data and data length */
	if (fit_image_get_data(fit, image_noffset, &data, &size)) {
		err_msg = "Can't get image data/size";
		goto error;
	}

	/* Process all hash subnodes of the component image node */
	fdt_for_each_subnode(noffset, fit, image_noffset) {
		const char *name = fit_get_name(fit, noffset, NULL);

		/*
		 * Check subnode name, must be equal to "hash".
		 * Multiple hash nodes require unique unit node
		 * names, e.g. hash@1, hash@2, etc.
		 */
		if (!strncmp(name, FIT_HASH_NODENAME, strlen(FIT_HASH_NODENAME))) {
			if (fit_image_check_hash(fit, noffset, data, size, &err_msg)){
				goto error;
			}
		} 
	}

	if (noffset == -FDT_ERR_TRUNCATED || noffset == -FDT_ERR_BADSTRUCTURE) {
		err_msg = "Corrupted or truncated tree";
		goto error;
	}

	return 1;

error:
	syslog(LOG_INFO, "error!\n%s for '%s' hash node in '%s' image node\n",
	       err_msg, fit_get_name(fit, noffset, NULL),
	       fit_get_name(fit, image_noffset, NULL));
	return 0;
}

/**
 * fit_all_image_verify - verify data integrity for all images
 * @fit: pointer to the FIT format image header
 *
 * fit_all_image_verify() goes over all images in the FIT and
 * for every images checks if all it's hashes are valid.
 *
 * returns:
 *     1, if all hashes of all images are valid
 *     0, otherwise (or on error)
 */
int fit_all_image_verify(const void *fit)
{
	int images_noffset;
	int noffset;
	int ndepth;
	int count;

	if(!fit){
		return 0;
	}

	/* Find images parent node offset */
	images_noffset = fdt_path_offset(fit, FIT_IMAGES_PATH);
	if (images_noffset < 0) {
		syslog(LOG_INFO, "Can't find images parent node '%s' (%s)\n", FIT_IMAGES_PATH, fdt_strerror(images_noffset));
		return 0;
	}

	/* Process all image subnodes, check hashes for each */
	for (ndepth = 0, count = 0,
	     noffset = fdt_next_node(fit, images_noffset, &ndepth);
			(noffset >= 0) && (ndepth > 0);
			noffset = fdt_next_node(fit, noffset, &ndepth)) {
		if (ndepth == 1) {
			/*
			 * Direct child node of the images parent node,
			 * i.e. component image node.
			 */
			//syslog(LOG_INFO, "Hash(es) for Image %u (%s): ", count, fit_get_name(fit, noffset, NULL));
			count++;

			if (!fit_image_verify(fit, noffset)){
				return 0;
			}
		}
	}

	return 1;
}

/**
 * fit_check_format - sanity check FIT image format
 * @fit: pointer to the FIT format image header
 *
 * fit_check_format() runs a basic sanity FIT image verification.
 * Routine checks for mandatory properties, nodes, etc.
 *
 * returns:
 *     1, on success
 *     0, on failure
 */
int fit_check_format(const void *fit)
{
	char desc[256];
	char *p = NULL;

	if(!fit){
		return 0;
	}

	p = (char *)fdt_getprop(fit, 0, FIT_DESC_PROP, NULL);
	/* mandatory / node 'description' property */
	if (p == NULL) {
		syslog(LOG_INFO, "Wrong image format: no description\n");
		return 0;
	}

	snprintf(desc, sizeof(desc), "%s", p);
	if(strstr(desc, FIT_IMAGE_DESC) == NULL){
		syslog(LOG_INFO, "Wrong image format: description is not match!\n");
		return 0;
	}

	/* mandatory subimages parent '/images' node */
	if (fdt_path_offset(fit, FIT_IMAGES_PATH) < 0) {
		syslog(LOG_INFO, "Wrong image format: no images node\n");
		return 0;
	}

	return 1;
}


