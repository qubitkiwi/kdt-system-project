#include "system_server.h"
#include <signal.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#include "../hal/camera_HAL.h"

pthread_mutex_t system_loop_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  system_loop_cond  = PTHREAD_COND_INITIALIZER;
bool            system_loop_exit = false;    ///< true if main loop should exit

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

void signal_exit(void) {
    /* 여기에 구현하세요..  종료 메시지를 보내도록.. */
    if (pthread_mutex_lock(&system_loop_mutex) != 0) {
        perror("system_loop_mutex");
        exit(-1);
    }

    system_loop_exit = true;

    if (pthread_mutex_unlock(&system_loop_mutex) != 0) {
        perror("system_loop_mutex");
        exit(-1);
    }
    pthread_cond_signal(&system_loop_cond);
}


static void timer_signal_handler(int sig, siginfo_t *si, void *uc) {
    timer++;
    signal_exit();
    // printf("system timer %d\n", timer);
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
    ts.it_value.tv_sec = 10;
    ts.it_value.tv_nsec = 0;
    ts.it_interval.tv_sec = 10;
    ts.it_interval.tv_nsec = 0;

    printf("start system server Timer\n");
    if (timer_settime(tidlist, 0, &ts, NULL) == -1) {
        perror("timer_settime");
        exit(-1);
    }    
}

void *camera_service_thread(void* arg) {
    char *s = arg;
    printf("%s start\n", s);

    toy_camera_open();
    toy_camera_take_picture();

    while (1) {
        posix_sleep_ms(5000);
    }
}

void *watchdog_thread(void* arg) {
    char *s = arg;
    printf("%s start\n", s);

    while (1) {
        sleep(1);
    }
}

void *disk_service_thread(void* arg) {
    char *s = arg;
    FILE* apipe;
    char buf[1024];
    char cmd[]="df -h ./";

    printf("%s start\n", s);

    while (1) {
        apipe = popen(cmd, "r");
        if (apipe == NULL) {
            perror("popen");
            continue;
        }
        while (fgets(buf, 1024, apipe) != NULL) {
            printf("%s", buf);
        }
        pclose(apipe);

        posix_sleep_ms(10000);
    }
}

void *monitor_thread(void* arg) {
    char *s = arg;
    printf("%s start\n", s);

    while (1) {
        sleep(1);
    }
}


void system_server() {
    printf("system_server Process\n");    

    init_sig_timer();

    // thread start
    pthread_t watchdog_thread_t, camera_service_thread_t, monitor_thread_t, disk_service_thread_t;

    if (pthread_create(&watchdog_thread_t, NULL, watchdog_thread, "watchdog thread") != 0) {
        perror("watchdog_thread");
        exit(-1);
    }
    if (pthread_create(&camera_service_thread_t, NULL, camera_service_thread, "camera service thread") != 0) {
        perror("camera_service_thread");
        exit(-1);
    }
    if (pthread_create(&monitor_thread_t, NULL, monitor_thread, "monitor thread") != 0) {
        perror("monitor_thread");
        exit(-1);
    }
    if (pthread_create(&disk_service_thread_t, NULL, disk_service_thread, "disk service thread") != 0) {
        perror("disk_service_thread");
        exit(-1);
    }

    if (pthread_mutex_lock(&system_loop_mutex) != 0) {
        perror("system_loop_mutex");
        exit(-1);
    }
    while (system_loop_exit == false){
        pthread_cond_wait(&system_loop_cond, &system_loop_mutex);
        system_loop_exit = true;    
    }
    if (pthread_mutex_unlock(&system_loop_mutex) != 0) {
        perror("system_loop_mutex");
        exit(-1);
    }
    printf("<== system\n");
    while (1) {
        sleep(1);
    }

    // thread end
    pthread_join(camera_service_thread_t, (void **)0);
    pthread_join(camera_service_thread_t, (void **)0);
    pthread_join(monitor_thread_t, (void **)0);
    pthread_join(disk_service_thread_t, (void **)0);

}