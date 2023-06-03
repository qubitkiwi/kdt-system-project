#ifndef _TOY_H
#define _TOY_H

#include "../../common.h"
#include <pthread.h>
#include <elf.h>

#define TOY_TOK_BUFSIZE 64
#define TOY_TOK_DELIM " \t\r\n\a"
#define TOY_BUFFSIZE 1024

#define CAMERA_TAKE_PICTURE 1
#define SENSOR_DATA 1
#define DUMP_STATE 2

typedef struct {
    unsigned int msg_type;
    unsigned int param1;
    unsigned int param2;
    void *param3;
} toy_msg_t;

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
int toy_message_queue(char **args);
int toy_read_elf_header(char **args);

void elf64_print (Elf64_Ehdr *elf_header);
int toy_dump_state();

#endif
