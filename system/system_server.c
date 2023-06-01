#include "system_server.h"
#include <signal.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <mqueue.h>

#include "../hal/camera_HAL.h"
#include "../ui/input/toy.h"

#define CAMERA_TAKE_PICTURE 1

pthread_mutex_t system_loop_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  system_loop_cond  = PTHREAD_COND_INITIALIZER;
bool            system_loop_exit = false;    ///< true if main loop should exit

static mqd_t system_queue[SERVER_THREAD_NUM];

const char *mq_dir[] = {
    WATCHDOG_QUEUE,
    MONITOR_QUEUE,
    DISK_QUEUE,
    CAMERA_QUEUE
};

const char *thread_name[] = {
    "watchdog",
    "monitor",
    "disk",
    "camera"
};

void* (*thread_function[SERVER_THREAD_NUM])(void*) = {
    (void* (*)(void*))watchdog_thread,
    (void* (*)(void*))monitor_thread,
    (void* (*)(void*))disk_service_thread,
    (void* (*)(void*))camera_service_thread
};

static int timer = 0;



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
    // intptr_t thread_id = (intptr_t)arg;
    int thread_id = (int)arg;
    toy_msg_t msg;
    printf("camera_service_thread start, id %d\n", thread_id);

    toy_camera_open();
    toy_camera_take_picture();

    while (1) {
        int num_read = mq_receive(system_queue[thread_id], (void*)&msg, sizeof(toy_msg_t), NULL);
        if (num_read < 0) continue;
        printf("camera_service_thread: 메시지가 도착했습니다.\n");
        printf("msg.type: %d\n", msg.msg_type);
        printf("msg.param1: %d\n", msg.param1);
        printf("msg.param2: %d\n", msg.param2);

        if (msg.msg_type == CAMERA_TAKE_PICTURE) {
            toy_camera_take_picture();
        }
    }
}

void *watchdog_thread(void* arg[]) {
    int thread_id = (int)arg;
    toy_msg_t msg;
    printf("watchdog_thread start id %d\n", thread_id);

    while (1) {
        int num_read = mq_receive(system_queue[thread_id], (void*)&msg, sizeof(toy_msg_t), NULL);
        if (num_read < 0) continue;
        printf("watchdog_thread: 메시지가 도착했습니다.\n");
        printf("msg.type: %d\n", msg.msg_type);
        printf("msg.param1: %d\n", msg.param1);
        printf("msg.param2: %d\n", msg.param2);
    }
}

void *disk_service_thread(void* arg) {
    int thread_id = (int)arg;
    toy_msg_t msg;
    printf("disk_service_thread start id %d\n", thread_id);

    FILE* apipe;
    char buf[1024];
    char cmd[]="df -h ./";
    
    while (1) {
        int num_read = mq_receive(system_queue[thread_id], (void*)&msg, sizeof(toy_msg_t), NULL);
        if (num_read < 0) continue;
        printf("watchdog_thread: 메시지가 도착했습니다.\n");
        printf("msg.type: %d\n", msg.msg_type);
        printf("msg.param1: %d\n", msg.param1);
        printf("msg.param2: %d\n", msg.param2);
    
        apipe = popen(cmd, "r");
        if (apipe == NULL) {
            perror("popen");
            continue;
        }
        while (fgets(buf, 1024, apipe) != NULL) {
            printf("%s", buf);
        }
        pclose(apipe);
    }
}

void *monitor_thread(void* arg) {
    int thread_id = (int)arg;
    toy_msg_t msg;
    printf("monitor_thread start %d\n", thread_id);

    while (1) {
        int num_read = mq_receive(system_queue[thread_id], (void*)&msg, sizeof(toy_msg_t), NULL);
        if (num_read < 0) continue;
        printf("monitor_thread: 메시지가 도착했습니다.\n");
        printf("msg.type: %d\n", msg.msg_type);
        printf("msg.param1: %d\n", msg.param1);
        printf("msg.param2: %d\n", msg.param2);
    }
}


void system_server() {
    printf("system_server Process\n");    

    init_sig_timer();

    for (int i=0; i<SERVER_THREAD_NUM; i++) {
        system_queue[i] = mq_open(mq_dir[i], O_RDWR);
        if (system_queue[i] == -1) {
            fprintf(stderr, "mq open err : %s\n", mq_dir[i]);
            exit(-1);
        }
    }

    // thread start
    pthread_t watchdog_thread_t, camera_service_thread_t, monitor_thread_t, disk_service_thread_t;

    pthread_t thread_id[SERVER_THREAD_NUM];
    for (int i=0; i<SERVER_THREAD_NUM; i++) {
        if (pthread_create(thread_id, NULL, thread_function[i], i) != 0) {
            fprintf(stderr,"%s thread err\n", thread_name);
            exit(-1);
        }        
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
    for (int i=0; i<SERVER_THREAD_NUM; i++) {
        pthread_join(thread_id[i], (void **)0);
        printf("end %d\n", i);
    }

}