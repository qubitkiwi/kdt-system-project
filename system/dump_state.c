#include "dump_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/klog.h>

#define KLOG_READ_ALL      3
#define KLOG_SIZE_BUFFER   10

static char* DUMP_CMD[] = {
    "/proc/version",
    "/proc/meminfo",
    "/proc/vmstat",
    "/proc/vmallocinfo",
    "/proc/slabinfo",
    "/proc/zoneinfo",
    "/proc/pagetypeinfo",
    "/proc/buddyinfo",
    "/proc/net/dev",
    "/proc/net/route",
    "/proc/net/ipv6_route",
    "/proc/interrupts",
    "dmesg",
    "show_wchan"
};

#define BUFFER_SIZE 1024

void dump_state_print(){
    for (int i=0; i<sizeof(DUMP_CMD)/sizeof(DUMP_CMD[0]); i++) {
        printf("==================== %d ==================\n", i);
        dump_file_print(DUMP_CMD[i]);
    }
    do_dmesg();
}

void dump_file_print(char *path) {

    FILE *file;
    char line[BUFFER_SIZE];

    file = fopen(path, "r");
    if (file == NULL) {
        perror("Error opening file");
        return ;
    }

    printf("==================== %s ==================\n", path);
    while (fgets(line, sizeof(line), file) != NULL) {
        // 줄을 처리하거나 출력합니다
        printf("%s", line);
    }
    printf("==============================================================\n");

    fclose(file);
}

void do_dmesg() {
    printf("------ KERNEL LOG (dmesg) ------\n");
    /* Get size of kernel buffer */
    int size = klogctl(KLOG_SIZE_BUFFER, NULL, 0);
    if (size <= 0) {
        printf("Unexpected klogctl return value: %d\n\n", size);
        return;
    }
    char *buf = (char *) malloc(size + 1);
    if (buf == NULL) {
        printf("memory allocation failed\n\n");
        return;
    }
    int retval = klogctl(KLOG_READ_ALL, buf, size);
    if (retval < 0) {
        printf("klogctl failure\n\n");
        free(buf);
        return;
    }
    buf[retval] = '\0';
    printf("%s\n\n", buf);
    free(buf);
    return;
}