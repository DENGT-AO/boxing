EXE = systools

#OBJS= gpio.o \
	iodebug.o \
	led.o \
	license.o \
	others.o \
	reboot_halt.o \
	systools.o \
	uboot.o
#	mtd.o sdebug.o rtc.o intools.o regexp.o

OBJS= systools.o \
	dnsc.o \
	rtc.o \
	reboot_halt.o \
	intools.o \
	uboot.o \
	led.o \
	mtd.o \
	iodebug.o \
	modemsignal.o \
	usbreset.o \
	iscfg.o \
	tipc-config.o \
	sdebug.o \
	stestserver.o \
	bspatch.o \
	devmem2.o 

ifeq ($(PRODUCT), IDTU9)
SYSTOOLS = dnsc rtc intools ledcon reboot mtd-erase mtd-write mtd-read iodebug bootenv iscfg tipc-config sdebug stestserver iburn comredirect rand get_rand devmem2 bspatch
else
SYSTOOLS = dnsc rtc intools ledcon reboot mtd-erase mtd-write mtd-read iodebug bootenv iscfg tipc-config sdebug stestserver iburn comredirect inpython devmem2 modemsignal usbreset
endif

ifneq ($(MOBIUS_PYTHON), y)
	OBJS += inpython.o
endif

ifneq ($(PRODUCT), IR9)
ifneq ($(MOBIUS_PYTHON), y)
INCLUDES_PATH += -I$(SYSTEM_PATH)/openssl2/include
LDLIBS += -L$(SYSTERM_PATH)/openssl2 -lcrypto 
OBJS += pysdk_install.o
SYSTOOLS += pysdk-install pyapp-install pyapp-uninstall
endif
endif

ifeq ($(strip $(PRODUCT)),$(filter $(PRODUCT),IG9 VG9 IG502 IG974))
INCLUDES_PATH += -I$(SYSTEM_PATH)/openssl2/include -I$(INHAND_PATH)/systools/libfdt
LDLIBS += -L$(SYSTERM_PATH)/openssl2 -L$(INHAND_PATH)/systools/libfdt -lcrypto -lfdt -ldl
OBJS += image-fit.o 
endif

ifeq ($(PRODUCT), IDTU9)
LDLIBS += -L$(INHAND_PATH)/randtest -lrand -lm -lcrypto  -L$(SYSTEM_PATH)/libbzip2 -lbz2
INCLUDES_PATH += -I$(SYSTEM_PATH)/libbzip2
endif

ifeq ($(strip $(PRODUCT)),$(filter $(PRODUCT),IG9 VG9 IG502 IG974))
OBJS += iburn_ig9.o
else
OBJS += iburn.o
endif

ifeq ($(MOBIUS_PYTHON), y)
OBJS += mobius_pysdk_flag.o mobius_pysdk_install.o mobius_pyapp_install.o
SYSTOOLS += mobius-pysdk-flag mobius-pysdk-install mobius-pyapp-install
endif

LDLIBS += $(LIB_SHARE) $(LIB_IPC) $(LIB_EVENT) -lm -ljansson

INCLUDES_PATH += -I$(SYSTEM_PATH)/jansson/src

include ../Makefile.rule

install_ex:
	@for s in $(SYSTOOLS); do \
		echo "create symbolic link $$s -> systools"; \
		if [ -L $(MY_INSTALL)/$$s ];then unlink $(MY_INSTALL)/$$s ;fi; \
		ln -s $(EXE) $(MY_INSTALL)/$$s; \
	done
	@install -m 0700 -D diagnose.sh $(MY_INSTALL)/diagnose.sh

