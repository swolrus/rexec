#include <sys/wait.h>
#include "./c-server.h"
#include "../common/strsplit.h"


int main(int argc, char *argv[])
{
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    if (argc < 2)
        exit(error("no port provided", EXIT_FAILURE));

    Host server = { .socket = -1, .hostname = NULL, .port = 6044 };

    h_connect(&server);
    listen(server.socket, 5);

    pthread_t t_id;

    int client;
    while (1) {
        if ((client = accept(server.socket, (struct sockaddr *) &cli_addr, &clilen)) < 0)
            exit(error("accept failed", EXIT_FAILURE));
        if (pthread_create(&t_id, NULL, client_handler, (void*)&client) < 0)
            exit(error("pthread_create() failed", EXIT_FAILURE));
    }
    close(server.socket);

    return 0;
}

int exec_set(ActionSet *as) {
    ExecPipes pipes[as->localcount];

    for (int i=0 ; i<as->localcount ; i++) {
        pipes[i] = exec_cmd(as->local[i]->cmd, "/");
    }
    pid_t wpid;
    char buf[BUF_SIZE];
    do {
        wpid = wait(NULL);
        for (int i=0 ; pipes[i].pid != wpid && i<as->localcount ; i++) {
            buf[0] = '\0';
            fgets(buf, BUF_SIZE, pipes[i].err);
            if (strlen(buf) > 1) {
                buf[strcspn(buf, "\n")] = '\0';
                as->local[i]->err = strdup(buf);
                as->local[i]->status = 1;
                return i;
            } else {
                buf[0] = '\0';
                fgets(buf, BUF_SIZE, pipes[i].out);
                buf[strcspn(buf, "\n")] = '\0';
                as->local[i]->out = strdup(buf);
                as->local[i]->status = 0;
            }
        }
    } while (wpid > 0);
    
    return 0;
}

void* client_handler(void* c) {
    pthread_detach(pthread_self());


    puts("A client has been connected");

    int client = *((int*)c);
    Message *msg;

    ActionSet *as = malloc(sizeof(char *));
    as = new_set("ACTIONS");

    int connected = 1;
    while (connected) {

        msg = recieve_data(client);

    //  RESPONS DEPENDANT ON MESSAGE
        switch (msg->header.type) {

        //  CLIENT WANTS CURRENT SERVER COST
            case CODE_COST: {
                send_data(client, "", CODE_COST);
                puts("sending cost");
                break;
            }

        //  CLIENT REQUESTING TO SEND AND HAVE EXECUTED AN ACTIONSET
            case CODE_AS_START: {
                as->remotecount = atoi(msg->data); // includes the count of actions in data
                send_data(client, "AWAITING COMMAND SET", CODE_OK);
                break;
            }

        //  RECIEVE SINGLE COMMAND
            case CODE_ACTION: {
                char *cmd = strdup(msg->data);
                msg = recieve_data(client);
            //  RECIEVE REQ COMMAND
                if (msg->header.type == CODE_REQ) {
                    add_cmd(as, cmd, msg->data);
                    if (strcmp(msg->data, "none") != 0) {
                        int count, recieved = 0;
                        char **req;
                        req = strsplit(&msg->data[8], &count);
                        //int recieved[count] = { 0 };
                        for (int i=0 ; i<count ; i++) {
                            printf("recieving file \"%s\"\n", req[i]);
                            if (recieve_file(client, req[i]) >= 0)
                                recieved++;
                            else
                                send_data(client, "RECIEVE FAILED", CODE_FAIL);
                        }
                    }
                    printf("[%d|%d] CMD: %s REQ: %s\n", as->remotecount, as->localcount, cmd, msg->data);
                } else {
                    send_data(client, "REQ NOT SUPPLIED FOR AS", CODE_FAIL);
                    connected = 0;
                    break;
                }
                break;
            }
        //  CLIENT SIGNALLING ALL ACTIONSET COMMANDS HAVE BEEN SENT
            case CODE_AS_END: {
            //  ENSURE WE HAVE RECIEVED ALL ACTION SETS
                if (as->remotecount == as->localcount) {
                    send_data(client, "RECIEVED, EXECUTING", CODE_OK);
                    int failed;
                    if ((failed = exec_set(as)) > 0) {
                        puts("actionset finished executing unsuccessfully");
                        send_data(client, as->local[failed]->err, CODE_ACTION_STATUS + failed);
                    } else {
                        puts("actionset finished executing successfully");
                        send_data(client, "EXEC COMPLETE", CODE_OK);
                    }
                connected = 0;
            //  OTHERWISE ALERT CLIENT
                } else {
                    send_data(client, "INCOMPLETE SET", CODE_FAIL);
                }
                break;
            }
            
        }
    }

    puts("Client Disconnected");
    close(client);
    return NULL;
}
