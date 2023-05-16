#include "web_server.h"

pid_t create_web_server() {
    
    pid_t web_pid = fork();
    if (web_pid == -1) {
        perror("web fork");
        exit(-1);
    } else if (web_pid == 0) {
        web_server();
    }
    return web_pid;
}

void web_server() {
    printf("web server 프로세스!\n");

    while (1) {
        sleep(1);
    }
}