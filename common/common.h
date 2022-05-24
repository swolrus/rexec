#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/types.h>

#if     defined(__linux__)
extern  char    *strdup(const char *str);
#endif

pid_t vfork(void);

// TYPE DEFINITION
#define BUF_SIZE 1024
#define ERROR_PREFIX "Error: "

// Enums
typedef enum {
    CODE_COST = 500,
    CODE_AS_START = 900,
    CODE_ACTION = 901,
    CODE_REQ = 902,
    CODE_AS_END = 903,
    CODE_FILE = 800,
    CODE_FILE_END = 801,
    CODE_ACTION_STATUS = 1000,
    CODE_FAIL = 99,
    CODE_OK = 200
} MESSAGE_TYPE;

// STRUCTS

// RAKEFILE STRUCTS

typedef struct Rake {
    char *filename;
    int port;
    struct Host **hosts;
    int hostcount;
    struct ActionSet **sets;
    int setcount;
} Rake;

typedef struct ActionSet {
    int socket;
    char *name; // id from heading
    struct Command **local; // local commands
    int localcount;
    struct Command **remote; // remote commands
    int remotecount;
}ActionSet;

typedef struct Command {
    char *cmd;
    char *req;
    int status;
    char *out;
    char *err;
}Command;

typedef struct ExecPipes {
    FILE * out, * err, * in;
    pid_t pid;
}ExecPipes;

typedef struct Host {
    int socket; // id if socket is connected else -1
    char *hostname;
    int port;
} Host;

typedef struct HEADER { // Fixed size header
    uint32_t type;
    uint32_t length;
} HEADER;

typedef struct Message { // Fixed size header
    struct HEADER header;
    char *data;
} Message;

// main.c
extern int              error(char *, int);
extern ExecPipes        exec_cmd(char *, char *);
extern ActionSet *      new_set(char *);
extern void             free_set(ActionSet *);
extern void             print_set(ActionSet *, int);
extern void             add_cmd(ActionSet *, char *, char *);
extern void             print_cmd(Command *, int);
extern void             free_cmd(Command *);

// socket.c
extern void             print_message(Message *m);
extern Host *           new_host(char *, int);
extern void             print_host(Host *);
extern int              c_connect(Host *);
extern int              h_connect(Host *);
extern Message *        recieve_data(int);
extern void             send_data(int, char *, int);
extern int              recieve_file(int, char *);
extern int              send_file(int, char *);

#endif // COMMON_H_
