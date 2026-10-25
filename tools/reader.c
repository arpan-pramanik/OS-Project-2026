/**
 * reader.c - User-space companion utility for task_lister kernel module
 *
 * Demonstrates consuming procfs interfaces from user space and
 * performing client-side filtering and analytics.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define PROC_LINEAR "/proc/task_lister"
#define PROC_TREE   "/proc/task_lister_tree"

static void print_usage(const char *progname)
{
	printf("Usage: %s [OPTIONS]\n", progname);
	printf("Options:\n");
	printf("  -a, --all         Display all tasks (default)\n");
	printf("  -t, --tree        Display hierarchical process tree\n");
	printf("  -p, --pid <PID>   Filter by process ID\n");
	printf("  -c, --comm <NAME> Filter by command name (substring match)\n");
	printf("  -s, --state <C>   Filter by state character (R, S, D, Z, T, etc.)\n");
	printf("  -k, --kernel      Show only kernel threads\n");
	printf("  -u, --user        Show only user-space processes\n");
	printf("  -S, --stats       Show only summary statistics\n");
	printf("  -h, --help        Show this help message\n");
}

int main(int argc, char *argv[])
{
	bool show_tree = false;
	bool show_stats_only = false;
	int filter_pid = -1;
	const char *filter_comm = NULL;
	char filter_state = '\0';
	const char *filter_type = NULL;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			print_usage(argv[0]);
			return 0;
		} else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tree") == 0) {
			show_tree = true;
		} else if (strcmp(argv[i], "-S") == 0 || strcmp(argv[i], "--stats") == 0) {
			show_stats_only = true;
		} else if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--pid") == 0) && i + 1 < argc) {
			filter_pid = atoi(argv[++i]);
		} else if ((strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--comm") == 0) && i + 1 < argc) {
			filter_comm = argv[++i];
		} else if ((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--state") == 0) && i + 1 < argc) {
			filter_state = argv[++i][0];
		} else if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--kernel") == 0) {
			filter_type = "kernel";
		} else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--user") == 0) {
			filter_type = "user";
		} else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0) {
			/* default */
		} else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			print_usage(argv[0]);
			return 1;
		}
	}

	const char *filepath = show_tree ? PROC_TREE : PROC_LINEAR;
	FILE *fp = fopen(filepath, "r");
	if (!fp) {
		perror("fopen");
		fprintf(stderr, "\nFailed to open %s.\n", filepath);
		fprintf(stderr, "Ensure task_lister kernel module is loaded: sudo insmod task_lister.ko\n");
		return 1;
	}

	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	bool in_summary = false;

	if (show_tree) {
		while ((nread = getline(&line, &len, fp)) != -1) {
			printf("%s", line);
		}
	} else {
		while ((nread = getline(&line, &len, fp)) != -1) {
			if (strstr(line, "Summary:")) {
				in_summary = true;
			}

			if (show_stats_only) {
				if (in_summary)
					printf("%s", line);
				continue;
			}

			if (line[0] == '=' || strstr(line, "PID      TGID")) {
				printf("%s", line);
				continue;
			}

			if (in_summary) {
				printf("%s", line);
				continue;
			}

			int pid, tgid, ppid, prio, nice;
			unsigned int uid;
			char state;
			char type[16];
			char comm[128];

			int parsed = sscanf(line, "%d %d %d %u %c %d %d %15s %127s",
					    &pid, &tgid, &ppid, &uid, &state, &prio, &nice, type, comm);

			if (parsed >= 9) {
				if (filter_pid >= 0 && pid != filter_pid)
					continue;
				if (filter_comm && !strcasestr(comm, filter_comm))
					continue;
				if (filter_state && state != filter_state)
					continue;
				if (filter_type && strcmp(type, filter_type) != 0)
					continue;
			}

			printf("%s", line);
		}
	}

	free(line);
	fclose(fp);
	return 0;
}
