#include "system_server.h"

pid_t create_system_server() {
    pid_t system_pid;
    system_pid = fork();
    if (system_pid == -1) {
        perror("system fork");
        exit(-1);
    } else if (system_pid == 0) {
        system_server();
    }
    return system_pid;
}

void system_server() {
    printf("system_server 프로세스!\n");

    execl("/usr/local/bin/filebrowser", "filebrowser", "-a", "0.0.0.0", "-p", "8080", (char *) NULL);

    // while (1) {
    //     sleep(1);
    // }
}
