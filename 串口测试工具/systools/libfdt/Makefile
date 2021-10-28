EXE_LIB = 1

EXE = libfdt.so

OBJS += \
	fdt.o \
	fdt_ro.o \
	fdt_rw.o \
	fdt_strerror.o \
	fdt_sw.o \
	fdt_wip.o 

#	fdt_empty_tree.o \
#	fdt_addresses.o \
#	fdt_region.o \
#	fdt_overlay.o 

LDFLAGS+=-shared

INCLUDES_PATH += -I$(INHAND_PATH)/systools/libfdt
include ../../Makefile.rule
