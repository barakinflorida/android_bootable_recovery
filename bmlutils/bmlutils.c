#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <sys/wait.h>
#include <sys/limits.h>
#include <dirent.h>
#include <sys/stat.h>

#include <signal.h>
#include <sys/wait.h>

// This was pulled from bionic: The default system command always looks
// for shell in /system/bin/sh. This is bad.
#define _PATH_BSHELL "/sbin/sh"

static int
run_exec_process ( char **argv) {
    pid_t pid;
    int status;
    pid = fork();
    if (pid == 0) {
        execv(argv[0], argv);
        fprintf(stderr, "E:Can't run (%s)\n",strerror(errno));
        _exit(-1);
    }

    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        return 1;
    }
    return 0;
}

#define BMLWRITE_BIN      "/sbin/bmlwrite"

int write_raw_image(const char* partition, const char* filename) {
    if (0 != strcmp("boot", partition)) {
        return -1;
    }
    char *const bmlwrite[] = {BMLWRITE_BIN, filename, BOARD_BOOT_DEVICE, NULL};
    return run_exec_process(bmlwrite);
}

int read_raw_image(const char* partition, const char* filename) {
    char _if[PATH_MAX];
    char _of[PATH_MAX];
    sprintf(_if, "if=%s", BOARD_BOOT_DEVICE);
    sprintf(_of, "of=%s", filename);
    char *const dd[] = {"/sbin/dd", _if, _of, NULL};
    return run_exec_process(dd);
}
