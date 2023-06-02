#ifndef _INPUT_H
#define _INPUT_H

#include "../common.h"
#include "./input/toy.h"
#include <signal.h>
#include <ucontext.h>
#include <execinfo.h>   

typedef struct _sig_ucontext {
    unsigned long uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    struct sigcontext uc_mcontext;
    sigset_t uc_sigmask;
} sig_ucontext_t;

int create_input();
void input();

#endif