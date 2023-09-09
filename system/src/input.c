#include "input.h"
#include "toy.h"
#include "sensor.h"
#include "system_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/shm.h>


static mqd_t system_queue[SERVER_QUEUE_NUM];


void segfault_handler(int sig_num, siginfo_t * info, void * ucontext) {
    void * array[50];
    void * caller_address;
    char ** messages;
    int size, i;
    sig_ucontext_t * uc;

    uc = (sig_ucontext_t *) ucontext;

    /* Get the address at the time the signal was raised */
    // rpi4
    caller_address = (void *) uc->uc_mcontext.pc;  // RIP: x86_64 specific     arm_pc: ARM
    // caller_address = (void *) uc->uc_mcontext.rip; // x86-64

    fprintf(stderr, "\n");

    if (sig_num == SIGSEGV)
        printf("signal %d (%s), address is %p from %p\n", sig_num, strsignal(sig_num), info->si_addr,
            (void *) caller_address);
    else
        printf("signal %d (%s)\n", sig_num, strsignal(sig_num));

    size = backtrace(array, 50);
    /* overwrite sigaction with caller's address */
    array[1] = caller_address;
    messages = backtrace_symbols(array, size);

    /* skip first stack frame (points here) */
    for (i = 1; i < size && messages != NULL; ++i) {
        printf("[bt]: (%d) %s\n", i, messages[i]);
    }

    free(messages);

    exit(EXIT_FAILURE);
}

pid_t create_input() {
    
    const char *name = "input";
    pid_t input_pid = fork();
    if (input_pid == -1) {
        perror("input fork");
        exit(-1);
    } else if (input_pid == 0) {
        if (prctl(PR_SET_NAME, (unsigned long) name) < 0) {
            perror("prctl()");
        }
        input_main();
    }
    return input_pid;
}

void *command_thread(void* arg) {
    char *s = arg;
    printf("%s start\n", s);

    toy_main();
    
    return NULL;
}

void *sensor_thread(void* arg) {
    char *s = arg;
    printf("%s start\n", s);
    toy_msg_t msg;
    int shm_id;
    BMP280_data_t *BMP280_data = NULL;

    shm_id = shmget((key_t)SHM_BMP280_KEY, sizeof(BMP280_data_t), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("shmget err");
        exit(-1);
    }
    BMP280_data = shmat(shm_id, NULL, 0);
    if (BMP280_data == (void *)-1) {
        perror("shmat err");
        exit(-1);
    }

    while (1) {
        posix_sleep_ms(5000);
        if (BMP280_data != NULL) {
            BMP280_data->temperature = rand()%40 - 10;
            BMP280_data->pressure = rand();
        }
        msg.msg_type = BMP280_SENSOR_DATA;
        msg.param1 = shm_id;
        msg.param2 = 0;

        printf("sensor_thread - BMP280_data : %d, %d\n", BMP280_data->temperature, BMP280_data->pressure);

        //MONITOR_QUEUE = system_queue[1]
        if (mq_send(system_queue[1], (char *)&msg, sizeof(msg), 0) == -1) {
            perror("mqretcode err");
        }
    }
}

void input_main() {
    printf("input Process\n");

    /* SIGSEGV 시그널 등록 */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sa.sa_sigaction = segfault_handler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction");
        exit(-1);
    }

    for (int i=0; i<SERVER_QUEUE_NUM; i++) {
        system_queue[i] = mq_open(mq_dir[i], O_RDWR);
        if (system_queue[i] == -1) {
            fprintf(stderr, "mq open err : %s\n", mq_dir[i]);
            exit(-1);
        }
    }

    // thread start
    pthread_t command_thread_t, sensor_thread_t;
    if (pthread_create(&command_thread_t, NULL, command_thread, "command thread") != 0) {
        perror("command_thread");
        exit(-1);
    }
    if (pthread_create(&sensor_thread_t, NULL, sensor_thread, "sensor thread") != 0) {
        perror("sensor_thread");
        exit(-1);
    }

    while (1) {
        sleep(1);
    }

    pthread_join(command_thread_t, (void **)0);
    pthread_join(sensor_thread_t, (void **)0);

}