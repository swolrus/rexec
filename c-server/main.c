#include <sys/wait.h>
#include "./c-server.h"


int main(int argc, char *argv[])
{
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    if (argc < 2)
        error("no port provided");

    Host server = { .socket = -1, .hostname = NULL, .port = 6044 };

    h_connect(&server);
    listen(server.socket, 5);

    pthread_t t_id;

    int client;
    while (1) {
        sleep(0.25);
        if ((client = accept(server.socket, (struct sockaddr *) &cli_addr, &clilen)) < 0)
            error("accept failed");
        if (pthread_create(&t_id, NULL, client_handler, (void*)&client) < 0)
            error("pthread_create() failed");
    }
    close(server.socket);

    return 0;
}

int exec_set(char **commands, int count) {
    pid_t cpid, wpid;
    int wstatus;

    for (int i=0; i<count; i++) {
        if ((cpid = fork()) == 0) {
            char std[BUF_SIZE];
            char err[BUF_SIZE];
            
            ExecPipes pipes = exec_cmd(commands[i]);

            while (fgets(std, BUF_SIZE, pipes.out)) {
                printf("OUT : %s", std);
            }

            while (fgets(err, BUF_SIZE, pipes.err)) {
                printf("ERR : %s", err);
                exit(EXIT_FAILURE);
            }
            fclose(pipes.in);
            exit(EXIT_SUCCESS);
        }
    }
//  WAIT FOR ALL COMMANDS TO EXIT
    while ((wpid = wait(&wstatus)) > 0) {
        if (wstatus != EXIT_SUCCESS) {
            return wstatus;
        }
    }
    return 0;
}

void* client_handler(void* c) {
    pthread_detach(pthread_self());

    puts("A client has been connected");

    int client = *((int*)c);
    Message *msg;

    char **commands;
    commands = malloc(sizeof(char *));
    int count = 0, total = 0, connected = 1;
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
                total = atoi(msg->data); // includes the count of actions in data
                count = 0;
                send_data(client, "AWAITING COMMAND SET", CODE_OK);
                break;
            }

        //  RECIEVE SINGLE COMMAND
            case CODE_ACTION: {
                commands = realloc(commands, (count + 1) * sizeof(char *));
                commands[count++] = strdup(msg->data);
                printf("%d|%d\n", count, total);
                break;
            }
        //  CLIENT SIGNALLING ALL ACTIONSET COMMANDS HAVE BEEN SENT
            case CODE_AS_END: {
            //  ENSURE WE HAVE RECIEVED ALL ACTION SETS
                if (total == count) {
                    send_data(client, "RECIEVED, EXECUTING", CODE_OK);

                    if (exec_set(commands, total) > 0) {
                        puts("actionset finished executing unsuccessfully");
                        send_data(client, "EXEC FAILED", CODE_FAIL);
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
