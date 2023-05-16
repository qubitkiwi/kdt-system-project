#include "system_server.h"

pid_t create_system_server() {

    const char *name = "system_server"; 
    pid_t system_pid;
    system_pid = fork();
    if (system_pid == -1) {
        perror("system fork");
        exit(-1);
    } else if (system_pid == 0) {
        if (prctl(PR_SET_NAME, (unsigned long) name) < 0) {
            perror("prctl()");
        }
        system_server();
    }
    return system_pid;
}

void system_server() {
    printf("system_server Process\n");

    execl("/usr/local/bin/filebrowser", "filebrowser", "-a", "0.0.0.0", "-p", "8080", (char *) NULL);

    // while (1) {
    //     sleep(1);
    // }
}
