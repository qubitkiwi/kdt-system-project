#ifndef _INPUT_H
#define _INPUT_H

#include "posix_timer.h"
#include "toy.h"

#include <signal.h>
#include <ucontext.h>
#include <execinfo.h>
#include <pthread.h>

typedef struct _sig_ucontext {
    unsigned long uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    struct sigcontext uc_mcontext;
    sigset_t uc_sigmask;
} sig_ucontext_t;

pid_t create_input();
void input_main();

#endif