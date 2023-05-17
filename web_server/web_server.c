#include "web_server.h"
#include <sys/time.h>

static int toy_timer = 0;

pid_t create_web_server() {

    const char *name = "web_server";    
    pid_t web_pid = fork();
    if (web_pid == -1) {
        perror("web fork");
        exit(-1);
    } else if (web_pid == 0) {
        if (prctl(PR_SET_NAME, (unsigned long) name) < 0) {
            perror("prctl()");
        }
        web_server();
    }
    return web_pid;
}

void web_server() {
    printf("web server Process\n");

    execl("/usr/local/bin/filebrowser", "filebrowser", "-a", "0.0.0.0", "-p", "8080", (char *) NULL);
}