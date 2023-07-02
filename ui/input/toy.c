#include "toy.h"
#include <sys/mman.h>
#include <string.h>
#include <wait.h>
#include <sys/stat.h>
#include <seccomp.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include "../../system/system_server.h"


pthread_mutex_t global_message_mutex  = PTHREAD_MUTEX_INITIALIZER;
char global_message[TOY_BUFFSIZE];
static mqd_t system_queue[SERVER_QUEUE_NUM];

char *builtin_str[] = {
    "send",
    "mu",
    "sh",
    "mq",
    "elf",
    "dump",
    "mincore",
    "busy",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &toy_send,
    &toy_mutex,
    &toy_shell,
    &toy_message_queue,
    &toy_read_elf_header,
    &toy_dump_state,
    &toy_mincore,
    &toy_busy,
    &toy_exit
};

int toy_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

int toy_send(char **args) {
    printf("send message: %s\n", args[1]);

    return 1;
}

int toy_exit(char **args) {
    return 0;
}

int toy_shell(char **args)
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("toy");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("toy");
    } else {
        do
        {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int toy_execute(char **args)
{
    int i;

    if (args[0] == NULL) {
        return 1;
    }

    for (i = 0; i < toy_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return 1;
}

char *toy_read_line(void)
{
    char *line = NULL;
    ssize_t bufsize = 0;

    if (getline(&line, &bufsize, stdin) == -1) {
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);
        } else {
            perror(": getline\n");
            exit(EXIT_FAILURE);
        }
    }
    return line;
}

char **toy_split_line(char *line)
{
    int bufsize = TOY_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token, **tokens_backup;

    if (!tokens) {
        fprintf(stderr, "toy: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, TOY_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += TOY_TOK_BUFSIZE;
            tokens_backup = tokens;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens) {
                free(tokens_backup);
                fprintf(stderr, "toy: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, TOY_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

void toy_loop(void)
{
    char *line;
    char **args;
    int status;

    // lock mincore
    scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_ALLOW);
    if (ctx == NULL) {
        perror("seccomp_init err");
        exit(-1);
    }

    int rc = seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(mincore), 0);
    if (rc < 0) {
        perror("seccomp_rule_add err");
        exit(-1);
    }

    seccomp_export_pfc(ctx, 5);
    seccomp_export_bpf(ctx, 6);

    rc = seccomp_load(ctx);
    if (rc < 0) {
        perror("seccomp_load err");
        exit(-1);
    }

    seccomp_release(ctx);
    
    // mq_init(system_queue, O_RDWR);
    for (int i=0; i<SERVER_QUEUE_NUM; i++) {
        system_queue[i] = mq_open(mq_dir[i], O_RDWR);
        if (system_queue[i] == -1) {
            fprintf(stderr, "mq open err : %s\n", mq_dir[i]);
            exit(-1);
        }
    }

    

    do {
        if (pthread_mutex_lock(&global_message_mutex) != 0) {
            perror("pthread_mutex_lock");
            exit(-1);
        }
        printf("TOY>");
        if (pthread_mutex_unlock(&global_message_mutex) != 0) {
            perror("pthread_mutex_lock");
            exit(-1);
        }
        line = toy_read_line();
        args = toy_split_line(line);
        status = toy_execute(args);

        free(line);
        free(args);
    } while (status);
}

int toy_mutex(char **args) {
    if (args[1] == NULL) {
        return 1;
    }

    printf("save message: %s\n", args[1]);
    // 여기서 뮤텍스
    if (pthread_mutex_lock(&global_message_mutex) != 0) {
        perror("pthread_mutex_lock");
        exit(-1);
    }

    strcpy(global_message, args[1]);

    if (pthread_mutex_unlock(&global_message_mutex) != 0) {
        perror("pthread_mutex_lock");
        exit(-1);
    }

    return 1;
}

int toy_message_queue(char **args)
{
    int mqretcode;
    toy_msg_t msg;

    if (args[1] == NULL || args[2] == NULL) {
        return 1;
    }

    if (!strcmp(args[1], "camera")) {
        msg.msg_type = atoi(args[2]);
        msg.param1 = 0;
        msg.param2 = 0;
        mqretcode = mq_send(system_queue[3], (char *)&msg, sizeof(msg), 0);
        if (mqretcode == -1) {
            perror("mqretcode err");
        }
    }
    return 1;
}

int toy_read_elf_header(char **args) {
    int mqretcode;
    toy_msg_t msg;
    int in_fd;
    size_t contents_sz;
    struct stat sb;
    Elf64_Ehdr* map;


    in_fd = open("./sample/sample.elf", O_RDONLY);
	if (in_fd < 0) {
        perror("cannot open ./sample/sample.elf");
        return 1;
    }
    if (fstat(in_fd, &sb) < 0) {
        perror("fstat");
        return 1;
    }
    contents_sz = sb.st_size;

    map = (Elf64_Ehdr*)mmap(NULL, contents_sz, PROT_READ, MAP_PRIVATE, in_fd, 0);
    elf64_print(map);
    munmap(map, contents_sz);
    close(in_fd);
    return 1;
}

void elf64_print (Elf64_Ehdr *elf_header) {
    printf("ELF file information:\n");
    printf(" Class: %d\n", elf_header->e_ident[EI_CLASS]);
    printf(" Data: %d\n", elf_header->e_ident[EI_DATA]);
    printf(" Version: %d\n", elf_header->e_ident[EI_VERSION]);
    printf(" OS/ABI: %d\n", elf_header->e_ident[EI_OSABI]);
    printf(" Type: %d\n", elf_header->e_type);
    printf(" Machine: %d\n", elf_header->e_machine);
    printf(" Entry point address: %d\n", elf_header->e_entry);
    printf(" Section header offset: %d\n", elf_header->e_shoff);
    printf(" Number of section headers: %d\n", elf_header->e_shnum);
    printf(" Size of section headers: %d\n", elf_header->e_shentsize);
    printf(" Program header offset: %d\n", elf_header->e_phoff);
    printf(" Number of program headers: %d\n", elf_header->e_phnum);
    printf(" Size of program headers: %d\n", elf_header->e_phentsize);
}

int toy_dump_state(char **args) {

    int mqretcode;
    toy_msg_t msg;

    msg.msg_type = DUMP_STATE;
    msg.param1 = 0;
    msg.param2 = 0;
    // system_queue[3] == CAMERA_QUEUE
    mqretcode = mq_send(system_queue[3], (char *)&msg, sizeof(msg), 0);
    if (mqretcode == -1) {
        perror("mq_send err");
    }

    // system_queue[1] == MONITOR_QUEUE
    mqretcode = mq_send(system_queue[1], (char *)&msg, sizeof(msg), 0);
    if (mqretcode == -1) {
        perror("mq_send err");
    }
    
    return 1;
}

int toy_mincore(char **args)
{
    unsigned char vec[20];
    int res;
    size_t page = sysconf(_SC_PAGESIZE);
    void *addr = mmap(NULL, 20 * page, PROT_READ | PROT_WRITE,
                    MAP_NORESERVE | MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    res = mincore(addr, 10 * page, vec);
    if (res == -1) {
        perror("mincore == -1");
    }

    return 1;
}

int toy_busy(char **args)
{
    while (1)
        ;
    return 1;
}