#include "libfdt_env.h"
#include "fdt.h"
#include "libfdt.h"
#include "libfdt_internal.h"

/**
 * overlay_get_target_phandle - retrieves the target phandle of a fragment
 * @fdto: pointer to the device tree overlay blob
 * @fragment: node offset of the fragment in the overlay
 *
 * overlay_get_target_phandle() retrieves the target phandle of an
 * overlay fragment when that fragment uses a phandle (target
 * property) instead of a path (target-path property).
 *
 * returns:
 *      the phandle pointed by the target property
 *      0, if the phandle was not found
 *	-1, if the phandle was malformed
 */
static uint32_t overlay_get_target_phandle(const void *fdto, int fragment)
{
	const uint32_t *val;
	int len;

	val = fdt_getprop(fdto, fragment, "target", &len);
	if (!val)
		return 0;

	if ((len != sizeof(*val)) || (*val == (uint32_t)-1))
		return (uint32_t)-1;

	return fdt32_to_cpu(*val);
}

/**
 * overlay_get_target - retrieves the offset of a fragment's target
 * @fdt: Base device tree blob
 * @fdto: Device tree overlay blob
 * @fragment: node offset of the fragment in the overlay
 *
 * overlay_get_target() retrieves the target offset in the base
 * device tree of a fragment, no matter how the actual targetting is
 * done (through a phandle or a path)
 *
 * returns:
 *      the targetted node offset in the base device tree
 *      Negative error code on error
 */
static int overlay_get_target(const void *fdt, const void *fdto,
			      int fragment)
{
	uint32_t phandle;
	const char *path;
	int path_len;

	/* Try first to do a phandle based lookup */
	phandle = overlay_get_target_phandle(fdto, fragment);
	if (phandle == (uint32_t)-1)
		return -FDT_ERR_BADPHANDLE;

	if (phandle)
		return fdt_node_offset_by_phandle(fdt, phandle);

	/* And then a path based lookup */
	path = fdt_getprop(fdto, fragment, "target-path", &path_len);
	if (!path) {
		/*
		 * If we haven't found either a target or a
		 * target-path property in a node that contains a
		 * __overlay__ subnode (we wouldn't be called
		 * otherwise), consider it a improperly written
		 * overlay
		 */
		if (path_len == -FDT_ERR_NOTFOUND)
			return -FDT_ERR_BADOVERLAY;

		return path_len;
	}

	return fdt_path_offset(fdt, path);
}

/**
 * overlay_phandle_add_offset - Increases a phandle by an offset
 * @fdt: Base device tree blob
 * @node: Device tree overlay blob
 * @name: Name of the property to modify (phandle or linux,phandle)
 * @delta: offset to apply
 *
 * overlay_phandle_add_offset() increments a node phandle by a given
 * offset.
 *
 * returns:
 *      0 on success.
 *      Negative error code on error
 */
static int overlay_phandle_add_offset(void *fdt, int node,
				      const char *name, uint32_t delta)
{
	const uint32_t *val;
	uint32_t adj_val;
	int len;

	val = fdt_getprop(fdt, node, name, &len);
	if (!val)
		return len;

	if (len != sizeof(*val))
		return -FDT_ERR_BADPHANDLE;

	adj_val = fdt32_to_cpu(*val);
	if ((adj_val + delta) < adj_val)
		return -FDT_ERR_NOPHANDLES;

	adj_val += delta;
	if (adj_val == (uint32_t)-1)
		return -FDT_ERR_NOPHANDLES;

	return fdt_setprop_inplace_u32(fdt, node, name, adj_val);
}

/**
 * overlay_adjust_node_phandles - Offsets the phandles of a node
 * @fdto: Device tree overlay blob
 * @node: Offset of the node we want to adjust
 * @delta: Offset to shift the phandles of
 *
 * overlay_adjust_node_phandles() adds a constant to all the phandles
 * of a given node. This is mainly use as part of the overlay
 * application process, when we want to update all the overlay
 * phandles to not conflict with the overlays of the base device tree.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_adjust_node_phandles(void *fdto, int node,
					uint32_t delta)
{
	int child;
	int ret;

	ret = overlay_phandle_add_offset(fdto, node, "phandle", delta);
	if (ret && ret != -FDT_ERR_NOTFOUND)
		return ret;

	ret = overlay_phandle_add_offset(fdto, node, "linux,phandle", delta);
	if (ret && ret != -FDT_ERR_NOTFOUND)
		return ret;

	fdt_for_each_subnode(child, fdto, node) {
		ret = overlay_adjust_node_phandles(fdto, child, delta);
		if (ret)
			return ret;
	}

	return 0;
}

/**
 * overlay_adjust_local_phandles - Adjust the phandles of a whole overlay
 * @fdto: Device tree overlay blob
 * @delta: Offset to shift the phandles of
 *
 * overlay_adjust_local_phandles() adds a constant to all the
 * phandles of an overlay. This is mainly use as part of the overlay
 * application process, when we want to update all the overlay
 * phandles to not conflict with the overlays of the base device tree.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_adjust_local_phandles(void *fdto, uint32_t delta)
{
	/*
	 * Start adjusting the phandles from the overlay root
	 */
	return overlay_adjust_node_phandles(fdto, 0, delta);
}

/**
 * overlay_update_local_node_references - Adjust the overlay references
 * @fdto: Device tree overlay blob
 * @tree_node: Node offset of the node to operate on
 * @fixup_node: Node offset of the matching local fixups node
 * @delta: Offset to shift the phandles of
 *
 * overlay_update_local_nodes_references() update the phandles
 * pointing to a node within the device tree overlay by adding a
 * constant delta.
 *
 * This is mainly used as part of a device tree application process,
 * where you want the device tree overlays phandles to not conflict
 * with the ones from the base device tree before merging them.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_update_local_node_references(void *fdto,
						int tree_node,
						int fixup_node,
						uint32_t delta)
{
	int fixup_prop;
	int fixup_child;
	int ret;

	fdt_for_each_property_offset(fixup_prop, fdto, fixup_node) {
		const uint32_t *fixup_val;
		const char *tree_val;
		const char *name;
		int fixup_len;
		int tree_len;
		int i;

		fixup_val = fdt_getprop_by_offset(fdto, fixup_prop,
						  &name, &fixup_len);
		if (!fixup_val)
			return fixup_len;

		if (fixup_len % sizeof(uint32_t))
			return -FDT_ERR_BADOVERLAY;

		tree_val = fdt_getprop(fdto, tree_node, name, &tree_len);
		if (!tree_val) {
			if (tree_len == -FDT_ERR_NOTFOUND)
				return -FDT_ERR_BADOVERLAY;

			return tree_len;
		}

		for (i = 0; i < (fixup_len / sizeof(uint32_t)); i++) {
			uint32_t adj_val, poffset;

			poffset = fdt32_to_cpu(fixup_val[i]);

			/*
			 * phandles to fixup can be unaligned.
			 *
			 * Use a memcpy for the architectures that do
			 * not support unaligned accesses.
			 */
			memcpy(&adj_val, tree_val + poffset, sizeof(adj_val));

			adj_val = fdt32_to_cpu(adj_val);
			adj_val += delta;
			adj_val = cpu_to_fdt32(adj_val);

			ret = fdt_setprop_inplace_namelen_partial(fdto,
								  tree_node,
								  name,
								  strlen(name),
								  poffset,
								  &adj_val,
								  sizeof(adj_val));
			if (ret == -FDT_ERR_NOSPACE)
				return -FDT_ERR_BADOVERLAY;

			if (ret)
				return ret;
		}
	}

	fdt_for_each_subnode(fixup_child, fdto, fixup_node) {
		const char *fixup_child_name = fdt_get_name(fdto, fixup_child,
							    NULL);
		int tree_child;

		tree_child = fdt_subnode_offset(fdto, tree_node,
						fixup_child_name);
		if (ret == -FDT_ERR_NOTFOUND)
			return -FDT_ERR_BADOVERLAY;
		if (tree_child < 0)
			return tree_child;

		ret = overlay_update_local_node_references(fdto,
							   tree_child,
							   fixup_child,
							   delta);
		if (ret)
			return ret;
	}

	return 0;
}

/**
 * overlay_update_local_references - Adjust the overlay references
 * @fdto: Device tree overlay blob
 * @delta: Offset to shift the phandles of
 *
 * overlay_update_local_references() update all the phandles pointing
 * to a node within the device tree overlay by adding a constant
 * delta to not conflict with the base overlay.
 *
 * This is mainly used as part of a device tree application process,
 * where you want the device tree overlays phandles to not conflict
 * with the ones from the base device tree before merging them.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_update_local_references(void *fdto, uint32_t delta)
{
	int fixups;

	fixups = fdt_path_offset(fdto, "/__local_fixups__");
	if (fixups < 0) {
		/* There's no local phandles to adjust, bail out */
		if (fixups == -FDT_ERR_NOTFOUND)
			return 0;

		return fixups;
	}

	/*
	 * Update our local references from the root of the tree
	 */
	return overlay_update_local_node_references(fdto, 0, fixups,
						    delta);
}

/**
 * overlay_fixup_one_phandle - Set an overlay phandle to the base one
 * @fdt: Base Device Tree blob
 * @fdto: Device tree overlay blob
 * @symbols_off: Node offset of the symbols node in the base device tree
 * @path: Path to a node holding a phandle in the overlay
 * @path_len: number of path characters to consider
 * @name: Name of the property holding the phandle reference in the overlay
 * @name_len: number of name characters to consider
 * @poffset: Offset within the overlay property where the phandle is stored
 * @label: Label of the node referenced by the phandle
 *
 * overlay_fixup_one_phandle() resolves an overlay phandle pointing to
 * a node in the base device tree.
 *
 * This is part of the device tree overlay application process, when
 * you want all the phandles in the overlay to point to the actual
 * base dt nodes.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_fixup_one_phandle(void *fdt, void *fdto,
				     int symbols_off,
				     const char *path, uint32_t path_len,
				     const char *name, uint32_t name_len,
				     int poffset, const char *label)
{
	const char *symbol_path;
	uint32_t phandle;
	int symbol_off, fixup_off;
	int prop_len;

	symbol_path = fdt_getprop(fdt, symbols_off, label,
				  &prop_len);
	if (!symbol_path)
		return prop_len;

	symbol_off = fdt_path_offset(fdt, symbol_path);
	if (symbol_off < 0)
		return symbol_off;

	phandle = fdt_get_phandle(fdt, symbol_off);
	if (!phandle)
		return -FDT_ERR_NOTFOUND;

	fixup_off = fdt_path_offset_namelen(fdto, path, path_len);
	if (fixup_off == -FDT_ERR_NOTFOUND)
		return -FDT_ERR_BADOVERLAY;
	if (fixup_off < 0)
		return fixup_off;

	phandle = cpu_to_fdt32(phandle);
	return fdt_setprop_inplace_namelen_partial(fdto, fixup_off,
						   name, name_len, poffset,
						   &phandle, sizeof(phandle));
};

/**
 * overlay_fixup_phandle - Set an overlay phandle to the base one
 * @fdt: Base Device Tree blob
 * @fdto: Device tree overlay blob
 * @symbols_off: Node offset of the symbols node in the base device tree
 * @property: Property offset in the overlay holding the list of fixups
 *
 * overlay_fixup_phandle() resolves all the overlay phandles pointed
 * to in a __fixups__ property, and updates them to match the phandles
 * in use in the base device tree.
 *
 * This is part of the device tree overlay application process, when
 * you want all the phandles in the overlay to point to the actual
 * base dt nodes.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_fixup_phandle(void *fdt, void *fdto, int symbols_off,
				 int property)
{
	const char *value;
	const char *label;
	int len;

	value = fdt_getprop_by_offset(fdto, property,
				      &label, &len);
	if (!value) {
		if (len == -FDT_ERR_NOTFOUND)
			return -FDT_ERR_INTERNAL;

		return len;
	}

	do {
		const char *path, *name, *fixup_end;
		const char *fixup_str = value;
		uint32_t path_len, name_len;
		uint32_t fixup_len;
		char *sep, *endptr;
		int poffset, ret;

		fixup_end = memchr(value, '\0', len);
		if (!fixup_end)
			return -FDT_ERR_BADOVERLAY;
		fixup_len = fixup_end - fixup_str;

		len -= fixup_len + 1;
		value += fixup_len + 1;

		path = fixup_str;
		sep = memchr(fixup_str, ':', fixup_len);
		if (!sep || *sep != ':')
			return -FDT_ERR_BADOVERLAY;

		path_len = sep - path;
		if (path_len == (fixup_len - 1))
			return -FDT_ERR_BADOVERLAY;

		fixup_len -= path_len + 1;
		name = sep + 1;
		sep = memchr(name, ':', fixup_len);
		if (!sep || *sep != ':')
			return -FDT_ERR_BADOVERLAY;

		name_len = sep - name;
		if (!name_len)
			return -FDT_ERR_BADOVERLAY;

		poffset = strtoul(sep + 1, &endptr, 10);
		if ((*endptr != '\0') || (endptr <= (sep + 1)))
			return -FDT_ERR_BADOVERLAY;

		ret = overlay_fixup_one_phandle(fdt, fdto, symbols_off,
						path, path_len, name, name_len,
						poffset, label);
		if (ret)
			return ret;
	} while (len > 0);

	return 0;
}

/**
 * overlay_fixup_phandles - Resolve the overlay phandles to the base
 *                          device tree
 * @fdt: Base Device Tree blob
 * @fdto: Device tree overlay blob
 *
 * overlay_fixup_phandles() resolves all the overlay phandles pointing
 * to nodes in the base device tree.
 *
 * This is one of the steps of the device tree overlay application
 * process, when you want all the phandles in the overlay to point to
 * the actual base dt nodes.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_fixup_phandles(void *fdt, void *fdto)
{
	int fixups_off, symbols_off;
	int property;

	/* We can have overlays without any fixups */
	fixups_off = fdt_path_offset(fdto, "/__fixups__");
	if ((fixups_off < 0 && (fixups_off != -FDT_ERR_NOTFOUND)))
		return fixups_off;

	/* And base DTs without symbols */
	symbols_off = fdt_path_offset(fdt, "/__symbols__");
	if ((symbols_off < 0 && (symbols_off != -FDT_ERR_NOTFOUND)))
		return symbols_off;

	fdt_for_each_property_offset(property, fdto, fixups_off) {
		int ret;

		ret = overlay_fixup_phandle(fdt, fdto, symbols_off, property);
		if (ret)
			return ret;
	}

	return 0;
}

/**
 * overlay_apply_node - Merges a node into the base device tree
 * @fdt: Base Device Tree blob
 * @target: Node offset in the base device tree to apply the fragment to
 * @fdto: Device tree overlay blob
 * @node: Node offset in the overlay holding the changes to merge
 *
 * overlay_apply_node() merges a node into a target base device tree
 * node pointed.
 *
 * This is part of the final step in the device tree overlay
 * application process, when all the phandles have been adjusted and
 * resolved and you just have to merge overlay into the base device
 * tree.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_apply_node(void *fdt, int target,
			      void *fdto, int node)
{
	int property;
	int subnode;

	fdt_for_each_property_offset(property, fdto, node) {
		const char *name;
		const void *prop;
		int prop_len;
		int ret;

		prop = fdt_getprop_by_offset(fdto, property, &name,
					     &prop_len);
		if (prop_len == -FDT_ERR_NOTFOUND)
			return -FDT_ERR_INTERNAL;
		if (prop_len < 0)
			return prop_len;

		ret = fdt_setprop(fdt, target, name, prop, prop_len);
		if (ret)
			return ret;
	}

	fdt_for_each_subnode(subnode, fdto, node) {
		const char *name = fdt_get_name(fdto, subnode, NULL);
		int nnode;
		int ret;

		nnode = fdt_add_subnode(fdt, target, name);
		if (nnode == -FDT_ERR_EXISTS) {
			nnode = fdt_subnode_offset(fdt, target, name);
			if (nnode == -FDT_ERR_NOTFOUND)
				return -FDT_ERR_INTERNAL;
		}

		if (nnode < 0)
			return nnode;

		ret = overlay_apply_node(fdt, nnode, fdto, subnode);
		if (ret)
			return ret;
	}

	return 0;
}

/**
 * overlay_merge - Merge an overlay into its base device tree
 * @fdt: Base Device Tree blob
 * @fdto: Device tree overlay blob
 *
 * overlay_merge() merges an overlay into its base device tree.
 *
 * This is the final step in the device tree overlay application
 * process, when all the phandles have been adjusted and resolved and
 * you just have to merge overlay into the base device tree.
 *
 * returns:
 *      0 on success
 *      Negative error code on failure
 */
static int overlay_merge(void *fdt, void *fdto)
{
	int fragment;

	fdt_for_each_subnode(fragment, fdto, 0) {
		int overlay;
		int target;
		int ret;

		/*
		 * Each fragments will have an __overlay__ node. If
		 * they don't, it's not supposed to be merged
		 */
		overlay = fdt_subnode_offset(fdto, fragment, "__overlay__");
		if (overlay == -FDT_ERR_NOTFOUND)
			continue;

		if (overlay < 0)
			return overlay;

		target = overlay_get_target(fdt, fdto, fragment);
		if (target < 0)
			return target;

		ret = overlay_apply_node(fdt, target, fdto, overlay);
		if (ret)
			return ret;
	}

	return 0;
}

int fdt_overlay_apply(void *fdt, void *fdto)
{
	uint32_t delta = fdt_get_max_phandle(fdt);
	int ret;

	FDT_CHECK_HEADER(fdt);
	FDT_CHECK_HEADER(fdto);

	ret = overlay_adjust_local_phandles(fdto, delta);
	if (ret)
		goto err;

	ret = overlay_update_local_references(fdto, delta);
	if (ret)
		goto err;

	ret = overlay_fixup_phandles(fdt, fdto);
	if (ret)
		goto err;

	ret = overlay_merge(fdt, fdto);
	if (ret)
		goto err;

	/*
	 * The overlay has been damaged, erase its magic.
	 */
	fdt_set_magic(fdto, ~0);

	return 0;

err:
	/*
	 * The overlay might have been damaged, erase its magic.
	 */
	fdt_set_magic(fdto, ~0);

	/*
	 * The base device tree might have been damaged, erase its
	 * magic.
	 */
	fdt_set_magic(fdt, ~0);

	return ret;
}
