#ifndef _TOY_H
#define _TOY_H

#include "../../common.h"
#include <pthread.h>

#define TOY_TOK_BUFSIZE 64
#define TOY_TOK_DELIM " \t\r\n\a"
#define TOY_BUFFSIZE 1024



int toy_send(char **args);
int toy_shell(char **args);
int toy_exit(char **args);

int toy_num_builtins();
int toy_send(char **args);
int toy_exit(char **args);
int toy_shell(char **args);
int toy_execute(char **args);
char *toy_read_line(void);
char **toy_split_line(char *line);

void toy_loop(void);

int toy_mutex(char **args);

#endif
