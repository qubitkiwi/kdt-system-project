#include "system_server.h"
#include <signal.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <mqueue.h>
#include <errno.h>

#include "../hal/camera_HAL.h"
#include "../ui/input/toy.h"
#include "../sensor.h"
#include <sys/shm.h>

#define CAMERA_TAKE_PICTURE 1
#define SENSOR_DATA 1

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
    "camera"
};

void* (*thread_function[SERVER_THREAD_NUM])(void*) = {
    (void* (*)(void*))watchdog_thread,
    (void* (*)(void*))monitor_thread,
    (void* (*)(void*))disk_service_thread,
    (void* (*)(void*))camera_service_thread,
    (void* (*)(void*))timer_thread
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
    int thread_id = (int)arg;
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
	return ;
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
        }
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
        if (pthread_create(thread_id, NULL, thread_function[i], i) != 0) {
            fprintf(stderr,"%s thread err\n", thread_name);
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