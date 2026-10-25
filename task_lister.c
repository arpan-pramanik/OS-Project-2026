// SPDX-License-Identifier: GPL-2.0-only
/**
 * task_lister.c - Linux Kernel Module for Listing All Current Tasks
 *
 * Demonstrates operating system process management concepts:
 * 1. Linear task iteration via circular doubly linked list (for_each_process)
 * 2. Hierarchical process tree traversal via Depth-First Search (DFS)
 * 3. Synchronization using Read-Copy-Update (RCU)
 * 4. User-space virtual filesystem integration using procfs seq_file
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/rcupdate.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/cred.h>
#include <linux/uidgid.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arpan Pramanik");
MODULE_DESCRIPTION("Linux Kernel Module for Listing All Current Tasks");
MODULE_VERSION("1.0.0");

#define PROC_FILENAME_LINEAR "task_lister"
#define PROC_FILENAME_TREE   "task_lister_tree"
#define MAX_TREE_DEPTH       64

/* Module parameter: "linear", "tree", or "both" */
static char *mode = "both";
module_param(mode, charp, 0444);
MODULE_PARM_DESC(mode, "Listing mode for kernel log: 'linear', 'tree', or 'both' (default: 'both')");

/* Statistics counters */
struct task_stats {
	unsigned int total;
	unsigned int user_tasks;
	unsigned int kernel_threads;
	unsigned int running;
	unsigned int sleeping;
	unsigned int disk_sleep;
	unsigned int stopped;
	unsigned int zombie;
	unsigned int other;
};

static void update_task_stats(struct task_struct *task, struct task_stats *stats)
{
	char state_ch;

	stats->total++;
	if (task->mm)
		stats->user_tasks++;
	else
		stats->kernel_threads++;

	state_ch = task_state_to_char(task);
	switch (state_ch) {
	case 'R':
		stats->running++;
		break;
	case 'S':
		stats->sleeping++;
		break;
	case 'D':
		stats->disk_sleep++;
		break;
	case 'T':
	case 't':
		stats->stopped++;
		break;
	case 'Z':
	case 'X':
		stats->zombie++;
		break;
	default:
		stats->other++;
		break;
	}
}

/*
 * Linear Task Listing
 * Iterates through all active processes in the system using for_each_process().
 */
static void list_tasks_linear(struct seq_file *m, bool log_to_dmesg, struct task_stats *stats)
{
	struct task_struct *task;

	if (m) {
		seq_puts(m, "===================================================================================================\n");
		seq_printf(m, "%-8s %-8s %-8s %-6s %-6s %-6s %-8s %-8s %-24s\n",
			   "PID", "TGID", "PPID", "UID", "STATE", "PRIO", "NICE", "TYPE", "COMMAND");
		seq_puts(m, "===================================================================================================\n");
	}

	if (log_to_dmesg) {
		pr_info("[task_lister] ========================== LINEAR TASK LIST ==========================\n");
		pr_info("[task_lister] %-8s %-8s %-8s %-6s %-6s %-6s %-8s %-8s %-24s\n",
			"PID", "TGID", "PPID", "UID", "STATE", "PRIO", "NICE", "TYPE", "COMMAND");
		pr_info("[task_lister] ----------------------------------------------------------------------\n");
	}

	rcu_read_lock();
	for_each_process(task) {
		pid_t pid = task->pid;
		pid_t tgid = task->tgid;
		pid_t ppid = task->real_parent ? task->real_parent->pid : 0;
		uid_t uid = from_kuid(&init_user_ns, task_uid(task));
		char state_ch = task_state_to_char(task);
		int prio = task->prio;
		int nice = task_nice(task);
		const char *type_str = task->mm ? "user" : "kernel";

		if (stats)
			update_task_stats(task, stats);

		if (m) {
			seq_printf(m, "%-8d %-8d %-8d %-6u %-6c %-6d %-6d %-8s %-24s\n",
				   pid, tgid, ppid, uid, state_ch, prio, nice, type_str, task->comm);
		}

		if (log_to_dmesg) {
			pr_info("[task_lister] %-8d %-8d %-8d %-6u %-6c %-6d %-6d %-8s %-24s\n",
				pid, tgid, ppid, uid, state_ch, prio, nice, type_str, task->comm);
		}
	}
	rcu_read_unlock();

	if (m && stats) {
		seq_puts(m, "===================================================================================================\n");
		seq_printf(m, "Summary: Total Tasks: %u | User: %u | Kernel: %u | Running: %u | Sleeping: %u | DiskSleep: %u | Stopped: %u | Zombie: %u\n",
			   stats->total, stats->user_tasks, stats->kernel_threads,
			   stats->running, stats->sleeping, stats->disk_sleep,
			   stats->stopped, stats->zombie);
		seq_puts(m, "===================================================================================================\n");
	}

	if (log_to_dmesg && stats) {
		pr_info("[task_lister] Total: %u | User: %u | Kernel: %u | Running: %u | Sleeping: %u | DiskSleep: %u | Zombie: %u\n",
			stats->total, stats->user_tasks, stats->kernel_threads,
			stats->running, stats->sleeping, stats->disk_sleep, stats->zombie);
	}
}

/*
 * Hierarchical DFS Task Tree Traversal
 * Recursively traverses process children starting from root (&init_task).
 */
static void dfs_traverse_tasks(struct task_struct *task, struct seq_file *m,
			      bool log_to_dmesg, int depth, struct task_stats *stats)
{
	struct task_struct *child;
	char indent[128];
	int i, indent_pos = 0;
	pid_t pid = task->pid;
	pid_t ppid = task->real_parent ? task->real_parent->pid : 0;
	char state_ch = task_state_to_char(task);
	const char *type_str = task->mm ? "user" : "kthread";

	if (depth >= MAX_TREE_DEPTH)
		return;

	if (stats)
		update_task_stats(task, stats);

	/* Build indentation string */
	for (i = 0; i < depth && indent_pos < sizeof(indent) - 8; i++) {
		indent_pos += scnprintf(indent + indent_pos, sizeof(indent) - indent_pos, "|   ");
	}
	if (depth > 0 && indent_pos < sizeof(indent) - 8) {
		indent_pos += scnprintf(indent + indent_pos, sizeof(indent) - indent_pos, "|-- ");
	} else if (depth == 0) {
		indent[0] = '\0';
	}

	if (m) {
		seq_printf(m, "%s[%d] %s (State: %c, PPID: %d, %s)\n",
			   indent, pid, task->comm, state_ch, ppid, type_str);
	}

	if (log_to_dmesg) {
		pr_info("[task_lister] %s[%d] %s (State: %c, PPID: %d, %s)\n",
			indent, pid, task->comm, state_ch, ppid, type_str);
	}

	list_for_each_entry(child, &task->children, sibling) {
		dfs_traverse_tasks(child, m, log_to_dmesg, depth + 1, stats);
	}
}

static void list_tasks_tree(struct seq_file *m, bool log_to_dmesg, struct task_stats *stats)
{
	if (m) {
		seq_puts(m, "===================================================================================================\n");
		seq_puts(m, "PROCESS HIERARCHY TREE (DFS Traversal from init_task)\n");
		seq_puts(m, "===================================================================================================\n");
	}

	if (log_to_dmesg) {
		pr_info("[task_lister] ========================== PROCESS TREE (DFS) ==========================\n");
	}

	rcu_read_lock();
	dfs_traverse_tasks(&init_task, m, log_to_dmesg, 0, stats);
	rcu_read_unlock();

	if (m && stats) {
		seq_puts(m, "===================================================================================================\n");
		seq_printf(m, "Summary: Visited %u tasks in hierarchy.\n", stats->total);
		seq_puts(m, "===================================================================================================\n");
	}
}

/*
 * Procfs single_open callback for linear view
 */
static int proc_linear_show(struct seq_file *m, void *v)
{
	struct task_stats stats = { 0 };

	list_tasks_linear(m, false, &stats);
	return 0;
}

/*
 * Procfs single_open callback for tree view
 */
static int proc_tree_show(struct seq_file *m, void *v)
{
	struct task_stats stats = { 0 };

	list_tasks_tree(m, false, &stats);
	return 0;
}

static struct proc_dir_entry *entry_linear;
static struct proc_dir_entry *entry_tree;

static int __init task_lister_init(void)
{
	struct task_stats stats = { 0 };
	bool do_linear = (strcmp(mode, "linear") == 0 || strcmp(mode, "both") == 0);
	bool do_tree   = (strcmp(mode, "tree") == 0 || strcmp(mode, "both") == 0);

	pr_info("[task_lister] Initializing Task Lister Kernel Module (mode: %s)\n", mode);

	/* Register procfs entries */
	entry_linear = proc_create_single(PROC_FILENAME_LINEAR, 0444, NULL, proc_linear_show);
	if (!entry_linear) {
		pr_err("[task_lister] Failed to create /proc/%s\n", PROC_FILENAME_LINEAR);
		return -ENOMEM;
	}

	entry_tree = proc_create_single(PROC_FILENAME_TREE, 0444, NULL, proc_tree_show);
	if (!entry_tree) {
		pr_err("[task_lister] Failed to create /proc/%s\n", PROC_FILENAME_TREE);
		proc_remove(entry_linear);
		return -ENOMEM;
	}

	pr_info("[task_lister] Created /proc/%s and /proc/%s\n",
		PROC_FILENAME_LINEAR, PROC_FILENAME_TREE);

	/* Output to dmesg ring buffer based on mode parameter */
	if (do_linear) {
		memset(&stats, 0, sizeof(stats));
		list_tasks_linear(NULL, true, &stats);
	}

	if (do_tree) {
		memset(&stats, 0, sizeof(stats));
		list_tasks_tree(NULL, true, &stats);
	}

	pr_info("[task_lister] Module loaded successfully.\n");
	pr_info("[task_lister] View real-time output via: cat /proc/%s or cat /proc/%s\n",
		PROC_FILENAME_LINEAR, PROC_FILENAME_TREE);

	return 0;
}

static void __exit task_lister_exit(void)
{
	pr_info("[task_lister] Unloading Task Lister Kernel Module...\n");

	if (entry_linear)
		proc_remove(entry_linear);

	if (entry_tree)
		proc_remove(entry_tree);

	pr_info("[task_lister] Cleaned up /proc entries. Unloaded successfully.\n");
}

module_init(task_lister_init);
module_exit(task_lister_exit);
