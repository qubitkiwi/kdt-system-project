#define _GNU_SOURCE
#include "system_server.h"
#include "toy.h"
#include "hardware.h"
#include "sensor.h"
#include "dump_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <semaphore.h>
#include <mqueue.h>
#include <errno.h>
#include <dirent.h>
#include <sched.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/time.h>



#define BUF_SIZE 1024
#define WATCH_DIR "./fs"

pthread_mutex_t system_loop_mutex   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t timer_mutex         = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  system_loop_cond    = PTHREAD_COND_INITIALIZER;
bool            system_loop_exit    = false;    ///< true if main loop should exit

static sem_t global_timer_sem;
static mqd_t system_queue[SERVER_QUEUE_NUM];
static int timer = 0;
static bool global_timer_stopped;

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
    "camera",
    "timer_thread",
    "engine_thread"
};

const int thread_ID[] = {0, 1, 2, 3, 4, 5};

void* (*thread_function[SERVER_THREAD_NUM])(void*) = {
    (void* (*)(void*))watchdog_thread,
    (void* (*)(void*))monitor_thread,
    (void* (*)(void*))disk_service_thread,
    (void* (*)(void*))camera_service_thread,
    (void* (*)(void*))timer_thread,
    (void* (*)(void*))engine_thread
};


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


static void timer_expire_signal_handler() {
    sem_post(&global_timer_sem);
}

static void system_timeout_handler() {
    pthread_mutex_lock(&timer_mutex);
    timer++;
    // printf("timer: %d\n", timer);
    pthread_mutex_unlock(&timer_mutex);
}

void set_periodic_timer(long sec_delay, long usec_delay) {
	struct itimerval itimer_val = {
		 .it_interval = { .tv_sec = sec_delay, .tv_usec = usec_delay },
		 .it_value = { .tv_sec = sec_delay, .tv_usec = usec_delay }
    };
	setitimer(ITIMER_REAL, &itimer_val, (struct itimerval*)0);
}

void *timer_thread(void *arg)
{
    int thread_id = *((int*)arg);
    printf("timer_thread start, id %d\n", thread_id);

    signal(SIGALRM, timer_expire_signal_handler);
    set_periodic_timer(1, 1);

	while (!global_timer_stopped) {
		int rc = sem_wait(&global_timer_sem);
		if (rc == -1 && errno == EINTR)
		    continue;

		if (rc == -1) {
		    perror("sem_wait");
		    exit(-1);
		}
		system_timeout_handler();
	}
    return NULL;
}

void *camera_service_thread(void* arg) {
    int thread_id = *((int*)arg);
    toy_msg_t msg;
    printf("camera_service_thread start, id %d\n", thread_id);

    int res;
    hw_module_t *module = NULL;
    res = hw_get_camera_module((const hw_module_t **)&module);
    if (res < 0) {
        perror("camera open fail");
        exit(-1);
    }
    module->open();
    // toy_camera_open();
    // toy_camera_take_picture();

    while (1) {
        int num_read = mq_receive(system_queue[thread_id], (void*)&msg, sizeof(toy_msg_t), NULL);
        if (num_read < 0) continue;
        printf("camera_service_thread: 메시지가 도착했습니다.\n");
        printf("msg.type: %d\n", msg.msg_type);
        printf("msg.param1: %d\n", msg.param1);
        printf("msg.param2: %d\n", msg.param2);

        if (msg.msg_type == CAMERA_TAKE_PICTURE) {
            // toy_camera_take_picture();
            module->take_picture();
        } else if (msg.msg_type == DUMP_STATE) {
            // toy_camera_dump();
            module->dump();
        } else {
            printf("camera_service_thread: unknown message %u", msg.msg_type);
        }
    }
}

void *watchdog_thread(void* arg[]) {
    int thread_id = *((int*)arg);
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

long long get_directory_size(const char *path) {
    struct stat st;
    long long total_size = 0;

    if (stat(path, &st) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (dir == NULL) {
            perror("opendir");
            exit(EXIT_FAILURE);
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char child_path[PATH_MAX];
            snprintf(child_path, PATH_MAX, "%s/%s", path, entry->d_name);

            total_size += get_directory_size(child_path);
        }

        closedir(dir);
    } else {
        total_size = st.st_size;
    }

    return total_size;
}

void *disk_service_thread(void* arg) {
    int thread_id = *((int*)arg);
    toy_msg_t msg;
    printf("disk_service_thread start id %d\n", thread_id);

    char buf[BUF_SIZE];
    long long total_size;
    int watch_fd;
    struct inotify_event* event;
    watch_fd = inotify_init();
    if (watch_fd == -1) {
        perror("inotify_init");
        return (void*)-1;
    }
    
    if (inotify_add_watch(watch_fd, WATCH_DIR, IN_CREATE) == -1) {
        perror("inotify_add_watch");
        return (void*)-1;
    }
  
    while (1) {
        int read_num = read(watch_fd, buf, BUF_SIZE);
        if (read_num == 0) {
            perror("read() from inotify fd returned 0!");
            continue;
        }
        if (read_num == -1) {
            perror("Read -1 bytes from inotify fd");
            continue;
        }

        for (char *p=buf; p < buf+read_num; ) {
            event = (struct inotify_event*) p;
            if (event ->mask & IN_CREATE) {
                printf("The file %s was created.\n", event->name);
            }
            p += sizeof(struct inotify_event) + event->len;
        }
        total_size = get_directory_size(WATCH_DIR);
        printf("dir size : %lld\n", total_size);
    }
}

void *monitor_thread(void* arg) {
    int thread_id = *((int*)arg);
    toy_msg_t msg;
    int shm_id, rev_shm_id;
    sensor_data_t *sensor_data = NULL;
    printf("monitor_thread start %d\n", thread_id);

    while (1) {
        int num_read = mq_receive(system_queue[thread_id], (void*)&msg, sizeof(toy_msg_t), NULL);
        if (num_read < 0) continue;
        printf("monitor_thread: 메시지가 도착했습니다.\n");
        printf("msg.type: %d\n", msg.msg_type);
        printf("msg.param1: %d\n", msg.param1);
        printf("msg.param2: %d\n", msg.param2);
        
        if (msg.msg_type == SENSOR_DATA) {
            rev_shm_id = msg.param1;
            sensor_data = shmat(rev_shm_id, NULL, 0);
            if (sensor_data == (void *)-1) {
                perror("shmat err");
                exit(-1);
            }
            printf("sensor temp: %d\n", sensor_data->temperature);
            printf("sensor pressure: %d\n", sensor_data->pressure);
            printf("sensor humidity: %d\n", sensor_data->humidity);
            if (shmdt(sensor_data) < 0) {
                perror("shmdt error");
                exit(-1);
            }
        } else if (msg.msg_type == DUMP_STATE) {
            dump_state_print();
        } else {
            printf("monitor_thread: unknown message %u", msg.msg_type);
        }
    }
}

void *engine_thread(void *arg)
{
    int thread_id = *((int*)arg);
    printf("engine_thread start id %d\n", thread_id);

    cpu_set_t set;
    struct sched_param sp;

    CPU_ZERO(&set);
    // 0번 cpu만 사용
    CPU_SET(0, &set);
    if (sched_setaffinity(gettid(), sizeof(set), &set) == -1) {
        perror("sched_setaffinity err");
        return (void*)-1;
    }

    sp.sched_priority = 50;
    if (sched_setscheduler(gettid(), SCHED_RR, &sp) == -1) {
        perror("sched_setscheduler err");
        return (void*)-1;
    }

    
    while (1) {
        posix_sleep_ms(10000);
    }
}

void system_server() {
    printf("system_server Process\n");    

    for (int i=0; i<SERVER_QUEUE_NUM; i++) {
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
        if (pthread_create(thread_id, NULL, thread_function[i], (void*)&thread_ID[i]) != 0) {
            fprintf(stderr,"%s thread err\n", thread_name[i]);
            exit(-1);
        }        
    }

    while (1) {
        sleep(1);
    }

    // thread end
    for (int i=0; i<SERVER_THREAD_NUM; i++) {
        pthread_join(thread_id[i], (void **)0);
        printf("end %d\n", i);
    }

}