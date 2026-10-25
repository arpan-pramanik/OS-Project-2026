#!/usr/bin/env bash
# unload.sh - Unload the task_lister kernel module

set -e

if ! lsmod | grep -q "^task_lister "; then
    echo "Module task_lister is not loaded."
    exit 0
fi

echo ">>> Unloading task_lister module..."
sudo rmmod task_lister

echo ">>> Module unloaded successfully."
echo ">>> Teardown logs:"
dmesg | grep "\[task_lister\]" | tail -n 10
