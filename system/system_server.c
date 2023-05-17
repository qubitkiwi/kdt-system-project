#include "system_server.h"
#include <signal.h>
#include <time.h>
#include <string.h>


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

static int timer = 0;

static void timer_signal_handler(int sig, siginfo_t *si, void *uc) {
    timer++;
    printf("system timer %d\n", timer);
}

void init_sig_timer() {
    // sig action
    struct sigaction  sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timer_signal_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGRTMAX, &sa, NULL) == -1) {
        perror("sigaction");
        exit(-1);
    }

    struct sigevent sev;
    timer_t tidlist;
    sev.sigev_notify = SIGEV_SIGNAL;    /* Notify via signal */
    sev.sigev_signo = SIGRTMAX;        /* Notify using this signal */

    sev.sigev_value.sival_ptr = &tidlist;
    if (timer_create(CLOCK_REALTIME, &sev, &tidlist) == -1) {
        perror("timer_create");
        exit(-1);
    }

    // itimer
    struct itimerspec ts;
    ts.it_value.tv_sec = 5;
    ts.it_value.tv_nsec = 0;
    ts.it_interval.tv_sec = 5;
    ts.it_interval.tv_nsec = 0;

    printf("start system server Timer\n");
    if (timer_settime(tidlist, 0, &ts, NULL) == -1) {
        perror("timer_settime");
        exit(-1);
    }    
}

void system_server() {
    printf("system_server Process\n");    
    
    init_sig_timer();

    while (1) {
        sleep(1);
    }
}