/*
 * libfdt - Flat Device Tree manipulation
 * Copyright (C) 2006 David Gibson, IBM Corporation.
 * SPDX-License-Identifier:	GPL-2.0+ BSD-2-Clause
 */
#include "libfdt_env.h"
#include "fdt.h"
#include "libfdt.h"

#include "libfdt_internal.h"

static int _fdt_sw_check_header(void *fdt)
{
	if (fdt_magic(fdt) != FDT_SW_MAGIC)
		return -FDT_ERR_BADMAGIC;
	/* FIXME: should check more details about the header state */
	return 0;
}

#define FDT_SW_CHECK_HEADER(fdt) \
	{ \
		int err; \
		if ((err = _fdt_sw_check_header(fdt)) != 0) \
			return err; \
	}

static void *_fdt_grab_space(void *fdt, size_t len)
{
	int offset = fdt_size_dt_struct(fdt);
	int spaceleft;

	spaceleft = fdt_totalsize(fdt) - fdt_off_dt_struct(fdt)
		- fdt_size_dt_strings(fdt);

	if ((offset + len < offset) || (offset + len > spaceleft))
		return NULL;

	fdt_set_size_dt_struct(fdt, offset + len);
	return _fdt_offset_ptr_w(fdt, offset);
}

int fdt_create(void *buf, int bufsize)
{
	void *fdt = buf;

	if (bufsize < sizeof(struct fdt_header))
		return -FDT_ERR_NOSPACE;

	memset(buf, 0, bufsize);

	fdt_set_magic(fdt, FDT_SW_MAGIC);
	fdt_set_version(fdt, FDT_LAST_SUPPORTED_VERSION);
	fdt_set_last_comp_version(fdt, FDT_FIRST_SUPPORTED_VERSION);
	fdt_set_totalsize(fdt,  bufsize);

	fdt_set_off_mem_rsvmap(fdt, FDT_ALIGN(sizeof(struct fdt_header),
					      sizeof(struct fdt_reserve_entry)));
	fdt_set_off_dt_struct(fdt, fdt_off_mem_rsvmap(fdt));
	fdt_set_off_dt_strings(fdt, bufsize);

	return 0;
}

int fdt_resize(void *fdt, void *buf, int bufsize)
{
	size_t headsize, tailsize;
	char *oldtail, *newtail;

	FDT_SW_CHECK_HEADER(fdt);

	headsize = fdt_off_dt_struct(fdt);
	tailsize = fdt_size_dt_strings(fdt);

	if ((headsize + tailsize) > bufsize)
		return -FDT_ERR_NOSPACE;

	oldtail = (char *)fdt + fdt_totalsize(fdt) - tailsize;
	newtail = (char *)buf + bufsize - tailsize;

	/* Two cases to avoid clobbering data if the old and new
	 * buffers partially overlap */
	if (buf <= fdt) {
		memmove(buf, fdt, headsize);
		memmove(newtail, oldtail, tailsize);
	} else {
		memmove(newtail, oldtail, tailsize);
		memmove(buf, fdt, headsize);
	}

	fdt_set_off_dt_strings(buf, bufsize);
	fdt_set_totalsize(buf, bufsize);

	return 0;
}

int fdt_add_reservemap_entry(void *fdt, uint64_t addr, uint64_t size)
{
	struct fdt_reserve_entry *re;
	int offset;

	FDT_SW_CHECK_HEADER(fdt);

	if (fdt_size_dt_struct(fdt))
		return -FDT_ERR_BADSTATE;

	offset = fdt_off_dt_struct(fdt);
	if ((offset + sizeof(*re)) > fdt_totalsize(fdt))
		return -FDT_ERR_NOSPACE;

	re = (struct fdt_reserve_entry *)((char *)fdt + offset);
	re->address = cpu_to_fdt64(addr);
	re->size = cpu_to_fdt64(size);

	fdt_set_off_dt_struct(fdt, offset + sizeof(*re));

	return 0;
}

int fdt_finish_reservemap(void *fdt)
{
	return fdt_add_reservemap_entry(fdt, 0, 0);
}

int fdt_begin_node(void *fdt, const char *name)
{
	struct fdt_node_header *nh;
	int namelen = strlen(name) + 1;

	FDT_SW_CHECK_HEADER(fdt);

	nh = _fdt_grab_space(fdt, sizeof(*nh) + FDT_TAGALIGN(namelen));
	if (! nh)
		return -FDT_ERR_NOSPACE;

	nh->tag = cpu_to_fdt32(FDT_BEGIN_NODE);
	memcpy(nh->name, name, namelen);
	return 0;
}

int fdt_end_node(void *fdt)
{
	fdt32_t *en;

	FDT_SW_CHECK_HEADER(fdt);

	en = _fdt_grab_space(fdt, FDT_TAGSIZE);
	if (! en)
		return -FDT_ERR_NOSPACE;

	*en = cpu_to_fdt32(FDT_END_NODE);
	return 0;
}

static int _fdt_find_add_string(void *fdt, const char *s)
{
	char *strtab = (char *)fdt + fdt_totalsize(fdt);
	const char *p;
	int strtabsize = fdt_size_dt_strings(fdt);
	int len = strlen(s) + 1;
	int struct_top, offset;

	p = _fdt_find_string(strtab - strtabsize, strtabsize, s);
	if (p)
		return p - strtab;

	/* Add it */
	offset = -strtabsize - len;
	struct_top = fdt_off_dt_struct(fdt) + fdt_size_dt_struct(fdt);
	if (fdt_totalsize(fdt) + offset < struct_top)
		return 0; /* no more room :( */

	memcpy(strtab + offset, s, len);
	fdt_set_size_dt_strings(fdt, strtabsize + len);
	return offset;
}

int fdt_property_placeholder(void *fdt, const char *name, int len, void **valp)
{
	struct fdt_property *prop;
	int nameoff;

	FDT_SW_CHECK_HEADER(fdt);

	nameoff = _fdt_find_add_string(fdt, name);
	if (nameoff == 0)
		return -FDT_ERR_NOSPACE;

	prop = _fdt_grab_space(fdt, sizeof(*prop) + FDT_TAGALIGN(len));
	if (! prop)
		return -FDT_ERR_NOSPACE;

	prop->tag = cpu_to_fdt32(FDT_PROP);
	prop->nameoff = cpu_to_fdt32(nameoff);
	prop->len = cpu_to_fdt32(len);
	*valp = prop->data;
	return 0;
}

int fdt_property(void *fdt, const char *name, const void *val, int len)
{
	void *ptr;
	int ret;

	ret = fdt_property_placeholder(fdt, name, len, &ptr);
	if (ret)
		return ret;
	memcpy(ptr, val, len);
	return 0;
}

int fdt_finish(void *fdt)
{
	char *p = (char *)fdt;
	fdt32_t *end;
	int oldstroffset, newstroffset;
	uint32_t tag;
	int offset, nextoffset;

	FDT_SW_CHECK_HEADER(fdt);

	/* Add terminator */
	end = _fdt_grab_space(fdt, sizeof(*end));
	if (! end)
		return -FDT_ERR_NOSPACE;
	*end = cpu_to_fdt32(FDT_END);

	/* Relocate the string table */
	oldstroffset = fdt_totalsize(fdt) - fdt_size_dt_strings(fdt);
	newstroffset = fdt_off_dt_struct(fdt) + fdt_size_dt_struct(fdt);
	memmove(p + newstroffset, p + oldstroffset, fdt_size_dt_strings(fdt));
	fdt_set_off_dt_strings(fdt, newstroffset);

	/* Walk the structure, correcting string offsets */
	offset = 0;
	while ((tag = fdt_next_tag(fdt, offset, &nextoffset)) != FDT_END) {
		if (tag == FDT_PROP) {
			struct fdt_property *prop =
				_fdt_offset_ptr_w(fdt, offset);
			int nameoff;

			nameoff = fdt32_to_cpu(prop->nameoff);
			nameoff += fdt_size_dt_strings(fdt);
			prop->nameoff = cpu_to_fdt32(nameoff);
		}
		offset = nextoffset;
	}
	if (nextoffset < 0)
		return nextoffset;

	/* Finally, adjust the header */
	fdt_set_totalsize(fdt, newstroffset + fdt_size_dt_strings(fdt));
	fdt_set_magic(fdt, FDT_MAGIC);
	return 0;
}

