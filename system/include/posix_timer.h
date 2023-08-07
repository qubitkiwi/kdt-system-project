#ifndef COMMON_H
#define COMMON_H

// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h> 

#include <sys/prctl.h>

int posix_sleep_ms(unsigned int timeout_ms);

#endif