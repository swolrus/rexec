#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#if     defined(__linux__)
extern  char    *strdup(const char *str);
#endif

pid_t vfork(void);

// TYPE DEFINITION
#define BUF_SIZE 1024 // or 1024 - 1
#define ERROR_PREFIX "Error: " // all errors prefix this
#define TMPDIR "tmp" // sub dir to create (must be containing directory of TMPDIR_FORMAT)
#define TMPDIR_FORMAT "tmp/as-XXXXXX" // format that mkdtemp uses when generating it's path

// Enums
typedef enum {
    CODE_EXIT = 0,
    CODE_FAIL = 99, // a request caused server exec to fail, close con
    CODE_BAD = 100, // nack
    CODE_OK = 200, // ack
    CODE_OUT_SZ = 300, // byte size of an output file about to be sent
    CODE_OUT = 301, // an output file chunk
    CODE_OUT_END = 302, // end of output file chunks
    CODE_ACTION_STATUS = 1000, // includes code>1000, will be STATUS + i where i is index of a command
    CODE_COST = 500, // client request cost
    CODE_AS_START = 900, // client has chosen host and wants to send actions/req
    CODE_ACTION = 901, // a action (command)
    CODE_REQ = 902, // a requirement (file) is wanting to be sent
    CODE_AS_EXECUTE = 904, // server safe to exec (all req/cmd have been sent)
    CODE_FILE = 800, // a character file line
    CODE_FILE_END = 801, // all character file lines have been sent
} CODES;

// STRUCTS

// RAKEFILE STRUCTS

typedef struct Rake { // container for a rakefile
    char *filename;
    int port;
    struct Host **hosts;
    int hostcount;
    struct ActionSet **sets;
    int setcount;
} Rake;

typedef struct ActionSet {
    int socket; // currently executing socket fd
    char *name; // id from heading
    struct Command **local; // local commands
    int localcount;
    struct Command **remote; // remote commands
    int remotecount;
}ActionSet;

typedef struct Command { // holds all information related to one command
    char *cmd;
    char *req;
    int status;
    char *out;
    char *err;
}Command;

typedef struct ExecPipes { // used to pass std and err pipes for execl
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
extern int              error(char *, int); // used to return and exit with a message
extern char *           new_tmpd(); // creates ./tmp if it doesn't exist and returns a new random subfolder
extern ExecPipes        exec_cmd(char *, char *);
extern ActionSet *      new_set(char *);
extern void             free_set(ActionSet *);
extern void             print_set(ActionSet *, bool);
extern void             add_cmd(ActionSet *, char *, char *); // test for 'remote-' and add new cmd struct to the action set
extern void             print_cmd(Command *, bool);
extern void             free_cmd(Command *);

// socket.c
extern void             print_message(Message *m); // print header and string of message structure
extern Host *           new_host(char *, int);
extern void             print_host(Host *);
extern int              c_connect(Host *); // connect to a socket
extern int              h_connect(Host *);
extern Message *        recieve_data(int); // recieve fixed length string and code
extern void             send_data(int, char *, int); 
extern int              recieve_file(int, char *); // send text files line by line
extern int              send_file(int, char *);
extern int              recieve_bfile(int, char *); // binary alternatives for .o objects
extern int              send_bfile(int, char *);

#endif // COMMON_H_
