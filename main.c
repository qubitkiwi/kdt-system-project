#include "./system/system_server.h"
#include "./ui/gui.h"
#include "./ui/input.h"
#include "./web_server/web_server.h"

#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

/* signal을 받으면 자식 프로세스를 종료한 뒤 종료 */
static void sigchldHandler(int sig) {
    int status, savedErrno;
    pid_t childPid;

    savedErrno = errno;         /* In case we modify 'errno' */

    printf("handler: Caught SIGCHLD\n");

    /* Do nonblocking waits until no more dead children are found */
    while ((childPid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("handler: Reaped child %ld - \n", (long) childPid);
        // (NULL, status);
    }
    if (childPid == -1 && errno != ECHILD) {
        perror("waitpid");
        exit(-1);
    }
    
    printf("handler: returning\n");

    errno = savedErrno;
}

int main() {

    pid_t system_pid, gui_pid, input_pid, web_pid;
    int status, savedErrno;

    /* SIGCHLD 시그널  등록 */
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sigchldHandler;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(-1);
    }
        
    //
    printf("main start\n");
    
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