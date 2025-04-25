obj-m += i2c-lcd-driver.o

# Use KERNEL_SRC if provided by Yocto, otherwise use the host kernel
KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Cross-compiler settings
CROSS_COMPILE ?= arm-poky-linux-gnueabi-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
AR = $(CROSS_COMPILE)ar
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
STRIP = $(CROSS_COMPILE)strip

# ARM-specific flags
ARCH = arm
CFLAGS_MODULE = -fno-pic

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) \
		ARCH=$(ARCH) \
		CC="$(CC)" \
		LD="$(LD)" \
		AR="$(AR)" \
		OBJCOPY="$(OBJCOPY)" \
		OBJDUMP="$(OBJDUMP)" \
		STRIP="$(STRIP)" \
		CROSS_COMPILE="$(CROSS_COMPILE)" \
		CFLAGS_MODULE="$(CFLAGS_MODULE)" \
		modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean 
