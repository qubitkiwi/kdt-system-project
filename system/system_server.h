#ifndef _SYSTEM_SERVER_H
#define _SYSTEM_SERVER_H

#include "../common.h"
#include <mqueue.h>

#define SERVER_THREAD_NUM 4
#define WATCHDOG_QUEUE  "/watchdog_queue"
#define MONITOR_QUEUE   "/monitor_queue"
#define DISK_QUEUE      "/disk_queue"
#define CAMERA_QUEUE    "/camera_queue"

extern const char *mq_dir[];



pid_t create_system_server();
void system_server();

void *camera_service_thread(void* arg);
void *watchdog_thread(void* arg[]);
void *disk_service_thread(void* arg);
void *monitor_thread(void* arg);


void signal_exit(void);
// static void timer_signal_handler(int sig, siginfo_t *si, void *uc);
void init_sig_timer();




#endif