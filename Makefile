# Makefile for Task Lister Linux Kernel Module & Tools

MODULE_NAME := task_lister
obj-m := $(MODULE_NAME).o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)
CC   ?= gcc
CFLAGS ?= -O2 -Wall -Wextra

# Default target: build kernel module and userspace companion reader
all: module tools

# Build kernel module
module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Build userspace companion reader
tools: tools/reader

tools/reader: tools/reader.c
	$(CC) $(CFLAGS) -o $@ $<

# Clean all artifacts
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod *.mod.c *.order *.symvers .*.cmd
	rm -rf .tmp_versions
	rm -f tools/reader

# Check kernel module metadata
info:
	@modinfo $(MODULE_NAME).ko

# Load kernel module (requires sudo)
load:
	@./scripts/load.sh

# Unload kernel module (requires sudo)
unload:
	@./scripts/unload.sh

# Run validation checks
test:
	@./scripts/verify.sh

# View module output from dmesg
dmesg:
	dmesg | grep "\[task_lister\]" | tail -n 50

# Quick view procfs files
cat-linear:
	@cat /proc/task_lister

cat-tree:
	@cat /proc/task_lister_tree

.PHONY: all module tools clean info load unload test dmesg cat-linear cat-tree
