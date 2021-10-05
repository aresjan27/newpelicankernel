#
# Makefile for the kernel character device drivers.
#

#
# This file contains the font map for the default (hardware) font
#

ifneq ($(KERNELRELEASE),)
	obj-$(CONFIG_R8125_PG) := pgdrv.o
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD :=$(shell pwd)

.PHONY:all
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

.PHONY:clean
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
endif