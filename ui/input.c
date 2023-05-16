#include "input.h"

pid_t create_input() {
    
    pid_t input_pid = fork();
    if (input_pid == -1) {
        perror("input fork");
        exit(-1);
    } else if (input_pid == 0) {
        input();
    }
    return input_pid;
}

void input() {
    printf("input 프로세스!\n");

    while (1) {
        sleep(1);
    }
}