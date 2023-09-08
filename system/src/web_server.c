#define _GNU_SOURCE

#include "web_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <sys/utsname.h>

#define STACK_SIZE (1024 * 1024)

pid_t create_web_server() {

    pid_t web_pid;

    char *stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED) {
        perror("mmap MAP_FAILED");
        exit(-1);
    }

    web_pid = clone(web_server, stack + STACK_SIZE, CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWPID | CLONE_NEWNS | SIGCHLD, NULL);
    if (web_pid == -1) {
        perror("clone err");
        exit(-1);
    }
    munmap(stack, STACK_SIZE);

    return web_pid;
}

int web_server()
{
    printf("web server Process\n");

    char path[1024];
    char *host_name = "web_server";
    struct utsname uts;

    if (getcwd(path, 1024) == NULL) {
        fprintf(stderr, "web server: current working directory get error: %s\n", strerror(errno));
        exit(-1);
    }
    if (sethostname(host_name, strlen(host_name)) == -1) {
        perror("web server : sethostname err");
        exit(-1);
    }
    if (uname(&uts) == -1) {
        perror("web server : uname err");
        exit(-1);
    }
        
        
    printf("%s : - [%4d] Current namspace, Parent PID : %d\n", host_name, getpid(), getppid() );
    printf("%s : current working directory: %s\n", host_name, path);
    printf("%s : hostname: %s\n", host_name, uts.nodename);

    execl("./web_server", "web_server", NULL);

    exit(0);
}