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
    printf(dirpath);

    if ((name = strrchr(dirpath, '/')) == NULL) {
        if ((name = strrchr(dirpath, '\\')) == NULL)
            name = fullpath;
    } else if (name != NULL) { // if still null (not a path)
        *name++ = '\0';
    }

    Rake *rake = rake_from_file(fullpath);
    print_rake(rake);
    printf("\nSTARTING EXECUTION\n");

    for (int i=0; i<rake->setCount; i++) {
        int wstatus;
        int pid;

        pid = fork();

        switch (pid) {
            case -1:
                exit(error("fork() failed", EXIT_FAILURE));
            case 0: {
                rake->sets[i]->socket = c_connect(rake->hosts[0]);
                negotiate_set(rake->sets[i], dirpath);
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


/**
 * @param as set to be executed
 * @param dirpath the directory to execute the command within
 * @return 0 if exec is success -1 if failed
 */
int negotiate_set(ActionSet *as, char *dirpath) {
    char pathbuf[BUF_SIZE];

    pid_t child_pid;
    int status = 0;

//  GET COST
    send_data(as->socket, "", CODE_COST);
    response = recieve_data(as->socket);
    print_message(response);

//  LOCAL COMMANDS

//  SPAWN LOCAL PARALLEL PROCESSES TO EXEC ACTIONSET
    for (int i=0 ; i<as->localCount ; i++) {
        if ((child_pid = fork()) == 0) {
            if ((status = system(as->local[i]->cmd)) != -1){
                //printf("%s // EXITED (%d)\n", as->local[i], WEXITSTATUS(status));
                exit(EXIT_SUCCESS);
            }
            exit(error("system() failed", EXIT_FAILURE));
        }
    }

//  CONFIRM START OF ACTIONSET
    uint32_t sz = (uint32_t) as->remotecount;
    response->header.type = CODE_AS_START;
    response->header.length = sizeof(uint32_t);
    if (send(as->socket, &response->header, sizeof(HEADER), 0) < 0) // send length
        exit(error("send() failed", EXIT_FAILURE));
    if (send(as->socket, &sz, sizeof(uint32_t), 0) < 0) // send length
        exit(error("send() failed", EXIT_FAILURE));

    response = recieve_data(as->socket);
    print_message(response);

    for (int i=0 ; i<as->remotecount ; i++) {
        send_data(as->socket, as->remote[i]->cmd, CODE_ACTION);
        if ((as->remote[i]->req) != NULL) {
            int count;
            char **wreq = NULL;
            wreq = strsplit(&as->remote[i]->req[8], &count);

            for (int i=0 ; i<count ; i++) {
                send_data(as->socket, wreq[i], CODE_REQ);

                response = recieve_data(as->socket);
                print_message(response);

                if (response->header.type == CODE_OK) {
                    memset(pathbuf, 0, sizeof(pathbuf));
                    snprintf(pathbuf, BUF_SIZE, "%s/%s", dirpath, wreq[i]);
                    printf("%s\n", pathbuf);
                    if ((strcmp(&wreq[i][strlen(wreq[i]) - 2], ".o")) == 0) {
                        if (send_bfile(as->socket, pathbuf) < 0) // we then want to send it back as it's an obj file
                            exit(error("send_file() failed", EXIT_FAILURE));
                    } else {
                        printf("Sending normal file");
                        if (send_file(as->socket, pathbuf) < 0)
                            exit(error("send_file() failed", EXIT_FAILURE));
                    }
                }
            }
        }
    }

    send_data(as->socket, "", CODE_AS_EXECUTE);
//  RECIEVE ACK THAT ACTIONSET COUNT MATCHES (MEANING REMOTE EXEC HAS STARTED)
    response = recieve_data(as->socket);
    print_message(response);


    while (1) {
        response = recieve_data(as->socket);
        print_message(response);
        if (response->header.type == CODE_OUT) {
            send_data(as->socket, "", CODE_OK);
            memset(pathbuf, 0, sizeof(pathbuf));
            snprintf(pathbuf, BUF_SIZE, "%s/%s", dirpath, response->data);
            if ((recieve_bfile(as->socket, pathbuf) < 0)) {
                send_data(as->socket, "FILE RECIEVE FAILED", CODE_FAIL);
                send_data(as->socket, "", CODE_EXIT);
                return -1;
                break;
            }
        } else if (response->header.type == CODE_OUT_END) {
            printf("\nRECIEVED RESULTS SUCCESSFULLY!\n");
            send_data(as->socket, "", CODE_EXIT);
            break;
        }
    }
    

    return 0;
}
