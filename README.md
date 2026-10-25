# Linux Kernel Module for Listing Tasks

A high-performance Linux Loadable Kernel Module (LKM) that inspects, enumerates, and analyzes all active tasks and processes in a running Linux operating system. Developed for the Operating Systems course curriculum.

---

## Overview

The `task_lister` kernel module demonstrates direct kernel-space inspection of the Operating System's Process Control Blocks (`struct task_struct`). It provides two complementary views of system activity:

1. **Linear Task List**: Iterates through the scheduler's global circular doubly linked list using the modern `for_each_process` macro under Read-Copy-Update (RCU) synchronization.
2. **Hierarchical Process Tree**: Traverses the parent-child-sibling process tree via Depth-First Search (DFS) starting from `init_task` (PID 0) down through all active user and kernel threads.

Output is exposed dynamically to user space through the Virtual File System (`/proc/task_lister` and `/proc/task_lister_tree`) using the kernel's memory-safe `seq_file` interface, as well as to the kernel ring buffer (`dmesg`).

---

## Key Features

- **Direct Kernel-Space Traversal**: Inspects actual `task_struct` instances in supervisor memory (Ring 0) rather than relying on multi-syscall user-space directory parsing.
- **Dual Traversal Modes**: Supports both flat table listing and recursive hierarchical tree visualization with depth-aware ASCII indentation.
- **Lock-Free RCU Synchronization**: Employs `rcu_read_lock()` and `rcu_read_unlock()` to ensure safe list traversal without taking heavy spinlocks or stalling concurrent `fork()` / `exit()` operations.
- **Procfs Virtual Filesystem Integration**: Creates `/proc/task_lister` and `/proc/task_lister_tree` via `proc_create_single()`, providing paginated, dynamically reallocating buffers that prevent page-boundary truncation.
- **Process Classification**: Automatically differentiates between User Space Processes (`task->mm != NULL`) and Kernel Threads (`task->mm == NULL`).
- **Comprehensive Process Metrics**: Extracts PID, Thread Group ID (TGID), Parent PID (PPID), User ID (UID), dynamic priority (`prio`), nice value (`task_nice`), scheduler state character ('R', 'S', 'D', 'Z', 'T'), and executable command name (`comm`).
- **Bounded Kernel Stack Usage**: Enforces a strict depth ceiling (`MAX_TREE_DEPTH = 64`) on recursive tree walks to protect the 16 KB kernel stack.
- **Companion User-Space Tool**: Includes `tools/reader`, a fast C utility for filtering by PID, command substring, execution state, or process type.
- **Automated Verification**: Includes `scripts/verify.sh` to validate module integrity and verify process count parity against standard `ps`.

---

## Project Structure

```text
OS-Project-2026/
├── Makefile                     # Standard Kbuild build system
├── task_lister.c                # Core kernel module source code
├── scripts/
│   ├── load.sh                  # Automated module build and load script
│   ├── unload.sh                # Module unload and cleanup script
│   └── verify.sh                # Test and parity verification suite
├── tools/
│   └── reader.c                 # User-space filtering and analytics CLI
├── docs/
│   ├── ARCHITECTURE.md          # Linux process subsystem architecture & PCB
│   ├── WORKING.md               # End-to-end operational lifecycle & flow
│   ├── PRINCIPLES.md            # Operating Systems theory & design principles
│   ├── HOW_IT_WORKS.md          # In-depth technical breakdown of macros & pointers
│   ├── PERFORMANCE_ANALYSIS.md  # Algorithmic complexity & syscall overhead analysis
│   └── USER_GUIDE.md            # Comprehensive user manual & troubleshooting
└── .gitignore                   # Kernel build artifact exclusion rules
```

---

## Comprehensive Documentation Index

For in-depth explanations and academic evaluation, refer to the specialized documentation files:

- [Architecture Guide](file:///home/arpan/Developments/OS-Project-2026/docs/ARCHITECTURE.md): Detailed breakdown of `struct task_struct`, thread groups, circular doubly linked lists, and process tree hierarchy.
- [Working & Lifecycle Guide](file:///home/arpan/Developments/OS-Project-2026/docs/WORKING.md): Step-by-step walkthrough of module initialization, procfs registration, traversal algorithms, and safe cleanup.
- [Operating System Principles](file:///home/arpan/Developments/OS-Project-2026/docs/PRINCIPLES.md): Theoretical analysis covering PCB abstraction, RCU synchronization, kernel stack constraints, and VFS mechanics.
- [Technical Deep Dive](file:///home/arpan/Developments/OS-Project-2026/docs/HOW_IT_WORKS.md): Low-level analysis of `for_each_process`, `container_of` pointer arithmetic, `sibling` lists, state bitmasks, and `seq_file`.
- [Performance Analysis](file:///home/arpan/Developments/OS-Project-2026/docs/PERFORMANCE_ANALYSIS.md): Big-O complexity calculations, kernel vs. user-space syscall overhead comparison, and cache locality.
- [User Guide](file:///home/arpan/Developments/OS-Project-2026/docs/USER_GUIDE.md): Practical instructions for compiling, loading, querying, testing, and resolving errors across different Linux distributions.

---

## Quick Start

### 1. Build Module and Companion Tool
```bash
make
```

### 2. Load the Kernel Module
```bash
# Load via script (requires sudo privileges)
./scripts/load.sh

# Or directly with make
make load
```

### 3. Inspect Kernel Ring Buffer Output
```bash
make dmesg
```

### 4. Query Real-Time Tasks via Procfs
```bash
# View flat process table
cat /proc/task_lister

# View hierarchical process tree
cat /proc/task_lister_tree
```

### 5. Use Companion CLI Reader
```bash
# View only running processes
./tools/reader --state R

# Filter by command name
./tools/reader --comm bash

# View system statistics summary
./tools/reader --stats
```

### 6. Run Parity Verification
```bash
make test
```

### 7. Unload Module
```bash
make unload
```

---

## Technical Specifications

- **Target Kernel**: Linux 5.14 through 6.x and 7.x (tested on Linux 7.2.9-arch1-1 x86_64)
- **License**: GPL-2.0
- **Language**: C (C99 / GNU extensions for kernel module, C99 for user space tool)
- **Toolchain**: GCC 14+ / GNU Make 4.4+ / Linux Kernel Build System (Kbuild)
