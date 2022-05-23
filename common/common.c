#include "common.h"

// CONSTANTS
#define BUF_SIZE 1024 // string buffer size

void error(char *msg) {
    fprintf(stderr, "%s%s\n", ERROR_PREFIX, msg);
    exit(EXIT_FAILURE);
}

void cleanup(int pipe_pair[2]) {
    close(pipe_pair[0]);
    close(pipe_pair[1]);
}

ExecPipes exec_cmd(char *command) {
    ExecPipes result = {.out = NULL, .in = NULL, .err = NULL};
    pid_t pid;

    int stdout_pair[2];
    int stdin_pair[2];
    int stderr_pair[2];

    if (pipe(stdout_pair) < 0) {
        return result;
    }
    if (pipe(stderr_pair) < 0) {
        cleanup(stdout_pair);
        return result;
    }
    if (pipe(stdin_pair) < 0) {
        cleanup(stdout_pair);
        cleanup(stderr_pair);
        return result;
    }

    pid = vfork(); // clone the process

    switch (pid) {
    //  FAIL CASE
        case -1: {
            cleanup(stdout_pair);
            cleanup(stderr_pair);
            cleanup(stdin_pair);

            error("fork() failed");
            break;
        }

    //  CHILD
        case 0: {
            dup2(stdout_pair[1], STDOUT_FILENO);
            cleanup(stdout_pair);

            dup2(stderr_pair[1], STDERR_FILENO);
            cleanup(stderr_pair);

            dup2(stdin_pair[0], STDIN_FILENO);
            cleanup(stdin_pair);

            execl("/bin/sh", "sh", "-c", command, NULL);
            exit(127);
            break;
        }

    //  PARENT
        default: {
            result.out = fdopen(stdout_pair[0], "r");
            close(stdout_pair[1]);

            result.err = fdopen(stderr_pair[0], "r");
            close(stderr_pair[1]);

            result.in = fdopen(stdin_pair[1], "w");
            close(stdin_pair[0]);

            return result;
        }

    } //close switch
    return result;
}
