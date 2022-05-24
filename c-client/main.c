#include "./c-client.h"
#include "../common/strsplit.h"

Message *response;

int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Usage : %s rakefilepath [rakefilepath2 ...]\n", argv[0]);
        //exit(EXIT_FAILURE);
    }
    char *fullpath = argv[1];
    char *dirpath = strdup(argv[1]);
    char *name = NULL;

    if ((name = strrchr(dirpath, '/')) == NULL) {
        if ((name = strrchr(dirpath, '\\')) == NULL)
            name = fullpath;
    } else if (name != NULL) { // if still null (not a path)
        *name++ = '\0';
    }

    Rake *rake = rake_from_file(fullpath);
    print_rake(rake);
    printf("\nSTARTING EXECUTION\n");

    for (int i=0; i<rake->setcount; i++) {
        int wstatus;
        int pid;

        pid = fork();

        switch (pid) {
            case -1:
                exit(error("fork() failed", EXIT_FAILURE));
            case 0: {
                rake->sets[i]->socket = c_connect(rake->hosts[0]);
                exec_local_set(rake->sets[i], dirpath);
                exit(EXIT_SUCCESS);
            }
            default:
            //  each actionset can only execute once previous is complete
                wait(&wstatus);
                break;
        }
    }

    return 0;
}


int exec_local_set(ActionSet *as, char *dirpath) {
//  GET COST
    send_data(as->socket, "", CODE_COST);
    response = recieve_data(as->socket);
    print_message(response);

//  LOCAL COMMANDS
    pid_t child_pid;
    int status = 0;

//  SPAWN LOCAL PARALLEL PROCESSES TO EXEC ACTIONSET
    for (int i=0 ; i<as->localcount ; i++) {
        if ((child_pid = fork()) == 0) {
            if ((status = system(as->local[i]->cmd)) != -1){
                //printf("%s // EXITED (%d)\n", as->local[i], WEXITSTATUS(status));
                exit(EXIT_SUCCESS);
            }
            exit(error("system() failed", EXIT_FAILURE));
        }
    }

//  CONFIRM START OF ACTIONSET
    char a[10];
    sprintf(a, "%d", as->remotecount);
    send_data(as->socket, a, CODE_AS_START);

    response = recieve_data(as->socket);
    print_message(response);

    for (int i=0 ; i<as->remotecount ; i++) {
        send_data(as->socket, as->remote[i]->cmd, CODE_ACTION);
        if ((as->remote[i]->req) != NULL) {
            int count;
            char **wreq = NULL;
            wreq = strsplit(&as->remote[i]->req[8], &count);

            char buffer[BUF_SIZE];
            for (int i=0 ; i<count ; i++) {
                send_data(as->socket, wreq[i], CODE_REQ);

                response = recieve_data(as->socket);
                print_message(response);

                if (response->header.type == CODE_OK) {
                    memset(buffer, 0, sizeof(buffer));
                    snprintf(buffer, BUF_SIZE, "%s/%s", dirpath, wreq[i]);
                    printf("%s\n", buffer);
                    if (send_file(as->socket, buffer) < 0)
                        exit(error("send_file() failed", EXIT_FAILURE));
                }
            }
        }
    }

    send_data(as->socket, "", CODE_AS_EXECUTE);
//  RECIEVE ACK THAT ACTIONSET COUNT MATCHES (MEANING REMOTE EXEC HAS STARTED)
    response = recieve_data(as->socket);
    print_message(response);

//  WAIT FOR REMOTE COMMANDS TO ALSO EXIT
    response = recieve_data(as->socket);
    print_message(response);

    while (1) {
        if (response->header.type == CODE_OUT) {
            send_data(as->socket, "", CODE_OK);
            if ((recieve_bfile(as->socket, response->data) < 0)) {
                send_data(as->socket, "FILE RECIEVE FAILED", CODE_FAIL);
                return -1;
            }
        } else if (response->header.type == CODE_OUT_END) {
            printf("\nRECIEVED RESULTS SUCCESSFULLY!\n");
            return 0;
        } else {
            return error("file transfer failed", -1); 
        }
    }

    return 0;
}
