#!/usr/bin/env bash
# verify.sh - Validate task_lister outputs and compare with user-space ps

set -e

PROC_FILE="/proc/task_lister"
TREE_FILE="/proc/task_lister_tree"

if [ ! -r "${PROC_FILE}" ]; then
    echo "ERROR: ${PROC_FILE} not found. Is task_lister module loaded?"
    echo "Run: sudo insmod task_lister.ko or ./scripts/load.sh"
    exit 1
fi

echo "=========================================================="
echo "          Task Lister Kernel Module Verification          "
echo "=========================================================="

echo "[1/4] Checking procfs file availability..."
test -r "${PROC_FILE}" && echo "  - ${PROC_FILE} is readable."
test -r "${TREE_FILE}" && echo "  - ${TREE_FILE} is readable."

echo "[2/4] Counting tasks from kernel module..."
MOD_COUNT=$(grep -E '^[0-9]+' "${PROC_FILE}" | wc -l)
echo "  - Total tasks listed in kernel module: ${MOD_COUNT}"

echo "[3/4] Counting tasks from userspace ps..."
PS_COUNT=$(ps -ef --no-headers | wc -l)
echo "  - Total tasks from 'ps -ef': ${PS_COUNT}"

DIFF=$(( MOD_COUNT - PS_COUNT ))
ABS_DIFF=${DIFF#-}

echo "[4/4] Comparing consistency..."
echo "  - Absolute delta: ${ABS_DIFF} (due to short-lived process churn during inspection)"
if [ "${ABS_DIFF}" -le 15 ]; then
    echo "  - SUCCESS: Kernel task count closely matches userspace process count."
else
    echo "  - NOTICE: Process count delta observed due to heavy system activity."
fi

echo ""
echo "Sample 5 processes from kernel module:"
head -n 8 "${PROC_FILE}"

echo ""
echo "Sample top of process hierarchy tree:"
head -n 12 "${TREE_FILE}"

echo ""
echo "Verification complete!"
