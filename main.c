#include "./system/system_server.h"
#include "./ui/gui.h"
#include "./ui/input.h"
#include "./web_server/web_server.h"

#include <stdio.h>
#include <sys/wait.h>


int main() {

    pid_t system_pid, gui_pid, input_pid, web_pid;
    int status, savedErrno;

    printf("main 함수 시작\n");
    
    printf("system server createing\n");
    system_pid = create_system_server();
    printf("gui createing\n");
    gui_pid = create_gui();
    printf("input createing\n");
    input_pid = create_input();
    printf("web server createing\n");
    web_pid = create_web_server();


    waitpid(system_pid, &status, 0);
    waitpid(gui_pid, &status, 0);
    waitpid(input_pid, &status, 0);
    waitpid(web_pid, &status, 0);

    return 0;
}