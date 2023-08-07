#ifndef _WEB_SERVER_H
#define _WEB_SERVER_H

#include "posix_timer.h"
#include "pthread.h"

pid_t create_web_server();
int web_server();

#endif