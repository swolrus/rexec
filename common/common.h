#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#if     defined(__linux__)
extern  char    *strdup(const char *str);
#endif

pid_t vfork(void);

// TYPE DEFINITION
#define BUF_SIZE 1024
#define ERROR_PREFIX "Error : "

// Enums
typedef enum {
    CODE_COST = 500,
    CODE_AS_START = 900,
    CODE_ACTION = 901,
    CODE_AS_END = 902,
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
    char **local; // local commands
    int localcount;
    char **remote; // remote commands
    int remotecount;
}ActionSet;


// SOCKET STRUCTS

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


// EXEC STRUCTS

typedef struct ExecPipes {
    FILE *in, *out, *err;
}ExecPipes;

extern void             error(char *);
extern ExecPipes        exec_cmd(char *);
extern int              c_connect(Host *);
extern int              h_connect(Host *);
extern Message *        recieve_data(int);
extern void             send_data(int , char *, int);
extern void             print_message(Message *m);
extern Host *           new_host(char *, int);
extern void             print_host(Host *);

#endif // COMMON_H_
