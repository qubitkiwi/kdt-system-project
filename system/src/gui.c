#include "gui.h"
#include "posix_timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>


pid_t create_gui() {
    
    const char *name = "gui"; 
    pid_t gui_pid;

    gui_pid = fork();
    if (gui_pid == -1) {
        perror("gui fork err");
        exit(-1);
    } else if (gui_pid == 0) {
        if (prctl(PR_SET_NAME, (unsigned long) name) < 0) {
            perror("gui prctl() err");
        }
        gui_main();
    }
    return gui_pid;
}

void gui_main() {
    printf("gui Process\n");

	// execl("/usr/bin/chromium-browser", "chromium-browser", "http://0.0.0.0:8080", NULL);
//    execl("/usr/bin/google-chrome-stable", "google-chrome-stable", "http://0.0.0.0:8000", NULL);
    exit(0);
}
