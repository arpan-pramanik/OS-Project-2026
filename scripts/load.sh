#!/usr/bin/env bash
# load.sh - Build and load the task_lister kernel module

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${PROJECT_DIR}"

MODE="${1:-both}"

echo ">>> Building kernel module..."
make

echo ">>> Loading task_lister module (mode=${MODE})..."
if lsmod | grep -q "^task_lister "; then
    echo "Module task_lister is already loaded. Reloading..."
    sudo rmmod task_lister
fi

sudo insmod task_lister.ko mode="${MODE}"

echo ">>> Module loaded successfully!"
echo ">>> Recent dmesg log:"
dmesg | grep "\[task_lister\]" | tail -n 25

echo ""
echo ">>> Available interfaces:"
echo "    - Tabular process view:  cat /proc/task_lister"
echo "    - Hierarchy tree view:   cat /proc/task_lister_tree"
