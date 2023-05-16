#include "gui.h"

pid_t create_gui() {
    
    const char *name = "gui"; 
    pid_t gui_pid = fork();
    if (gui_pid == -1) {
        perror("gui fork");
        exit(-1);
    } else if (gui_pid == 0) {
        if (prctl(PR_SET_NAME, (unsigned long) name) < 0) {
            perror("prctl()");
        }
        gui();
    }
    return gui_pid;
}

void gui() {
    printf("gui Process\n");

	execl("/usr/bin/chromium-browser", "chromium-browser", "http://0.0.0.0:8080", NULL);
//    execl("/usr/bin/google-chrome-stable", "google-chrome-stable", "http://0.0.0.0:8000", NULL);
    
}
