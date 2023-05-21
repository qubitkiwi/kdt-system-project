#include "input.h"
#include <signal.h>
#include <string.h>
#include <ucontext.h>
#include <execinfo.h>

extern pthread_mutex_t global_message_mutex;
extern char global_message[];

typedef struct _sig_ucontext {
    unsigned long uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    struct sigcontext uc_mcontext;
    sigset_t uc_sigmask;
} sig_ucontext_t;

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
        input();
    }
    return input_pid;
}

void *command_thread(void* arg) {
    char *s = arg;
    printf("%s start\n", s);

    toy_loop();
}

void *sensor_thread(void* arg) {
    char *s = arg;
    printf("%s start\n", s);

    char saved_message[TOY_BUFFSIZE];
    int i;
    while (1) {
        i = 0;

        if (pthread_mutex_lock(&global_message_mutex) != 0) {
            perror("pthread_mutex_lock");
            exit(-1);
        }
    
        while (global_message[i] != '\0') {
            printf("%c", global_message[i]);
            fflush(stdout);
            posix_sleep_ms(500);
            i++;
        }

        if (pthread_mutex_unlock(&global_message_mutex) != 0) {
            perror("pthread_mutex_lock");
            exit(-1);
        }

        posix_sleep_ms(500);
    }
}

void input() {
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