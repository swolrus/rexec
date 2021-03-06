#include "./common.h"

/**
 * clean error message with optional compact return
 * @param  msg the error message
 * @param  e   int to return, set to NULL if not req
 * @return     an int, can be used to exit with this function as param
 *                or to pass the appropriate return
 */
int error(char *msg, int e) {
    fprintf(stderr, "%s%s\n", ERROR_PREFIX, msg);
    return e;
}

/**
 * Helper to close pipes
 */
void cleanup(int pipePair[2]) {
    close(pipePair[0]);
    close(pipePair[1]);
}

/**
 * creates dir /tmp and then a randomly generated subdir
 * @return the path of the new dir
 */
char *new_tmpd() {
//  CREATE /TMP IF DOES NOT EXIST
    struct stat st = {0};
    if (stat(TMPDIR, &st) == -1) {
        if (mkdir(TMPDIR, S_IRWXU) == -1)
            exit(error("could not create /tmp", EXIT_FAILURE));
    }
//  CREATE DIR TO STORE FILES IN AND RETURN
    char *tPath = strdup(TMPDIR_FORMAT);
    if ((mkdtemp(tPath)) == NULL)
        exit(error("could not generate tmp directory", EXIT_FAILURE));
    return tPath;
}

/**
 * Allocate a new action set
 * @param  name of the set
 * @return      returns the new sets location
 */
ActionSet *new_set(char *name) {
    ActionSet *atmp = NULL;
    atmp = malloc(sizeof(ActionSet));
    atmp->name = strdup(name);
    atmp->localCount = 0;
    atmp->remoteCount = 0;
    atmp->local = malloc(1 * sizeof(Command *));
    atmp->remote = malloc(1 * sizeof(Command *));
    return atmp;
}

/**
 * free a sets memory
 */
void free_set(ActionSet *as) {
    free(as->name);
    free(as->local);
    free(as->remote);
    free(as);
}

/**
 * print a set
 * @param as     the set to print
 * @param remote if true will print a more verbose but more compact version
 */
void print_set(ActionSet *as, bool remote) {
    int i;
    printf("\n[ %s ]\n", as->name);
    printf("Local:\n");
    for (i=0 ; i<as->localCount ; i++) {
        if (remote)
            print_cmd(as->local[i], 1);
        else
            print_cmd(as->local[i], 0);
    }
    if (!remote) {
        printf("Remote:\n");
        for (i=0 ; i<as->remoteCount ; i++) {
            print_cmd(as->remote[i], 0);
        }
    }
}

/**
 * Will create a new command in the correct subset of actionset as
 * @param as       The target set
 * @param command  The command text
 * @param requires The requirements text
 */
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
        as->remote = realloc(as->remote, (as->remoteCount + 2) * sizeof(Command *));
        as->remote[as->remoteCount++] = cmd;
    } else {
    //  AS LOCAL
        as->local = realloc(as->local, (as->localCount + 2) * sizeof(Command *));
        as->local[as->localCount++] = cmd;
    }
    
}

/**
 * Free command memory
 */
void free_cmd(Command *cmd) {
    free(cmd->cmd);
    free(cmd->req);
    free(cmd->err);
    free(cmd->out);
    free(cmd);
}

/**
 * print a command
 * @param cmd the command struct to be printed
 * @param req if 1 will print a more verbose version for when errors are present
 */
void print_cmd(Command *cmd, bool req) {
    if (req == 0)
        printf("-%s\n  - %s\n", cmd->cmd, cmd->req);
    else
        printf("[%d](%s)\nOUT: %s\nERR: %s\n", cmd->status, cmd->cmd, cmd->out, cmd->err);
}

/**
 * create a child to execute a command via execl
 * also link its stdout/stderr to pipes accessible from the parent
 * 
 * @param  command the command text to execute
 * @param  path    the path to execute the command at (relative)
 * @return         a struct containing the pipes
 */
ExecPipes exec_cmd(char *cmd, char *path) {
    printf("EXEC complete %s @ %s\n", cmd, path);
    ExecPipes result = {.out = NULL, .err = NULL, .in=NULL};
    //int wstatus;

    int stdoutPair[2];
    // int stdinPair[2];
    int stderrPair[2];

    if (pipe(stdoutPair) < 0) {
        return result;
    }
    if (pipe(stderrPair) < 0) {
        cleanup(stdoutPair);
        return result;
    }
    // if (pipe(stdinPair) < 0) {
    //     cleanup(stdoutPair);
    //     cleanup(stderrPair);
    //     return result;
    // }

    result.pid = vfork(); // clone the process

    switch (result.pid) {
    //  FAIL CASE
        case -1: {
            cleanup(stdoutPair);
            cleanup(stderrPair);
            //cleanup(stdinPair);

            exit(error("fork() failed", EXIT_FAILURE));
            break;
        }

    //  CHILD
        case 0: {
            dup2(stdoutPair[1], STDOUT_FILENO);
            cleanup(stdoutPair);

            dup2(stderrPair[1], STDERR_FILENO);
            cleanup(stderrPair);

            // dup2(stdinPair[0], STDIN_FILENO);
            // cleanup(stdinPair);

            char buffer[BUF_SIZE];
            snprintf(buffer, BUF_SIZE, "cd %s; %s", path, cmd);

            execl("/bin/sh", "sh", "-c", buffer, NULL);

            exit(error("execl() failed", EXIT_FAILURE));
            break;
        }

    //  PARENT
        default: {

            result.out = fdopen(stdoutPair[0], "r");
            close(stdoutPair[1]);

            result.err = fdopen(stderrPair[0], "r");
            close(stderrPair[1]);

            // in = fdopen(stdinPair[1], "w");
            // close(stdinPair[0]);
            // fclose(in);

            return result;
        }

    } //close switch
    return result;
}
