# Makefile for Task Lister Linux Kernel Module
# Target module
MODULE_NAME := task_lister
obj-m := $(MODULE_NAME).o

# Kernel build directory
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Default target: build kernel module
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Clean build artifacts
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod *.mod.c *.order *.symvers .*.cmd
	rm -rf .tmp_versions

# Check module info
info:
	@modinfo $(MODULE_NAME).ko

# Load module (requires root/sudo)
load:
	sudo insmod $(MODULE_NAME).ko

# Unload module (requires root/sudo)
unload:
	sudo rmmod $(MODULE_NAME)

# View module output from dmesg
dmesg:
	dmesg | grep "\[task_lister\]" | tail -n 50

# View procfs outputs
cat-linear:
	@cat /proc/task_lister

cat-tree:
	@cat /proc/task_lister_tree

.PHONY: all clean info load unload dmesg cat-linear cat-tree
