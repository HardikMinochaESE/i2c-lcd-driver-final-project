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

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) \
		CC="$(CC)" \
		LD="$(LD)" \
		AR="$(AR)" \
		OBJCOPY="$(OBJCOPY)" \
		OBJDUMP="$(OBJDUMP)" \
		STRIP="$(STRIP)" \
		CROSS_COMPILE="$(CROSS_COMPILE)" \
		modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean 
