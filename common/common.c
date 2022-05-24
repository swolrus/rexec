#include "./common.h"


int error(char *msg, int e) {
    fprintf(stderr, "%s%s\n", ERROR_PREFIX, msg);
    return e;
}

void cleanup(int pipe_pair[2]) {
    close(pipe_pair[0]);
    close(pipe_pair[1]);
}

char *new_tmpd() {
//  CREATE /TMP IF DOES NOT EXIST
    struct stat st = {0};
    if (stat(TMPDIR, &st) == -1) {
        if (mkdir(TMPDIR, S_IRWXU) == -1)
            exit(error("could not create /tmp", EXIT_FAILURE));
    }
//  CREATE DIR TO STORE FILES IN AND RETURN
    char *tpath = strdup(TMPDIR_FORMAT);
    if ((mkdtemp(tpath)) == NULL)
        exit(error("could not generate tmp directory", EXIT_FAILURE));
    return tpath;
}

ActionSet *new_set(char *name) {
    ActionSet *atmp = NULL;
    atmp = malloc(sizeof(ActionSet));
    atmp->name = strdup(name);
    atmp->localcount = 0;
    atmp->remotecount = 0;
    atmp->local = malloc(1 * sizeof(Command *));
    atmp->remote = malloc(1 * sizeof(Command *));
    return atmp;
}

void free_set(ActionSet *as) {
    free(as->name);
    free(as->local);
    free(as->remote);
    free(as);
}

void print_set(ActionSet *as, int remote) {
    int i;
    printf("\n[ %s ]\n", as->name);
    printf("Local:\n");
    for (i=0 ; i<as->localcount ; i++) {
        if (remote)
            print_cmd(as->local[i], 1);
        else
            print_cmd(as->local[i], 0);
    }
    if (!remote) {
        printf("Remote:\n");
        for (i=0 ; i<as->remotecount ; i++) {
            print_cmd(as->remote[i], 0);
        }
    }
}

void add_cmd(ActionSet *as, char *command, char *requires) {
    Command *cmd = NULL;
    cmd = malloc(sizeof(Command));
    cmd->status = -1;
    cmd->out = NULL;
    cmd->err = NULL;

//  SETUP CMD AND REQ STRINGS
    int remote = 0; // 0 = local, 1 = remote

    if (!strncmp(command, "remote-", 6)) {
    // TRUNCATE REMOTE PREFIX
        remote = 1; // is remote
        command = &command[7];
    }

    cmd->cmd = malloc((strlen(command) + 1) * sizeof(char));
    cmd->cmd = strdup(command);

//  STORE REQ (OR LEAVE NULL IF NONE)
    cmd->req = NULL;
    if (requires != NULL) {
        cmd->req = malloc((strlen(requires) + 1) * sizeof(char));
        cmd->req = strdup(requires);
    }

//  STORE COMMAND AND REQ IN APPROPRIATE ARRAY
    if (remote == 1) {
    //  AS REMOTE
        as->remote = realloc(as->remote, (as->remotecount + 2) * sizeof(Command *));
        as->remote[as->remotecount++] = cmd;
    } else {
    //  AS LOCAL
        as->local = realloc(as->local, (as->localcount + 2) * sizeof(Command *));
        as->local[as->localcount++] = cmd;
    }
    
}

void free_cmd(Command *cmd) {
    free(cmd->cmd);
    free(cmd->req);
    free(cmd->err);
    free(cmd->out);
    free(cmd);
}

void print_cmd(Command *cmd, int req) {
    if (req == 0)
        printf("-%s\n  - %s\n", cmd->cmd, cmd->req);
    else
        printf("[%d](%s)\nOUT: %s\nERR: %s\n", cmd->status, cmd->cmd, cmd->out, cmd->err);
}

ExecPipes exec_cmd(char *command, char *path) {
    ExecPipes result = {.out = NULL, .err = NULL, .in=NULL};
    //int wstatus;

    int stdout_pair[2];
    // int stdin_pair[2];
    int stderr_pair[2];

    if (pipe(stdout_pair) < 0) {
        return result;
    }
    if (pipe(stderr_pair) < 0) {
        cleanup(stdout_pair);
        return result;
    }
    // if (pipe(stdin_pair) < 0) {
    //     cleanup(stdout_pair);
    //     cleanup(stderr_pair);
    //     return result;
    // }

    result.pid = vfork(); // clone the process

    switch (result.pid) {
    //  FAIL CASE
        case -1: {
            cleanup(stdout_pair);
            cleanup(stderr_pair);
            //cleanup(stdin_pair);

            exit(error("fork() failed", EXIT_FAILURE));
            break;
        }

    //  CHILD
        case 0: {
            dup2(stdout_pair[1], STDOUT_FILENO);
            cleanup(stdout_pair);

            dup2(stderr_pair[1], STDERR_FILENO);
            cleanup(stderr_pair);

            // dup2(stdin_pair[0], STDIN_FILENO);
            // cleanup(stdin_pair);

            char buffer[BUF_SIZE];
            snprintf(buffer, BUF_SIZE, "cd %s; %s", path, command);

            execl("/bin/sh", "sh", "-c", buffer, NULL);

            exit(error("execl() failed", EXIT_FAILURE));
            break;
        }

    //  PARENT
        default: {

            result.out = fdopen(stdout_pair[0], "r");
            close(stdout_pair[1]);

            result.err = fdopen(stderr_pair[0], "r");
            close(stderr_pair[1]);

            // in = fdopen(stdin_pair[1], "w");
            // close(stdin_pair[0]);
            // fclose(in);

            return result;
        }

    } //close switch
    return result;
}
