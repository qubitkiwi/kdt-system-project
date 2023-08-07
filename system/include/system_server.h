#ifndef _SYSTEM_SERVER_H
#define _SYSTEM_SERVER_H

#include "posix_timer.h"
#include <mqueue.h>
#include <pthread.h>

#define SERVER_THREAD_NUM 6
#define SERVER_QUEUE_NUM 4
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
void *timer_thread(void *arg);
void *engine_thread(void *arg);

long long get_directory_size(const char *path);

#endif