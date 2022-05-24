#include <sys/wait.h>
#include <dirent.h>
#include "./c-server.h"
#include "../common/strsplit.h"


int main(int argc, char *argv[])
{
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    if (argc < 3)
        exit(error("no port provided", EXIT_FAILURE));

    Host server = { .socket = -1, .hostname = argv[1], .port = atoi(argv[2]) };

    h_connect(&server); // populates our struct
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

int exec_set(ActionSet *as, char *tmpdir) {
    ExecPipes pipes[as->localcount];

    for (int i=0 ; i<as->localcount ; i++) {
        pipes[i] = exec_cmd(as->local[i]->cmd, tmpdir);
    }
    pid_t wpid;
    char buf[BUF_SIZE];
    int err = 0, wstatus;
    do {
        wpid = wait(&wstatus);
        for (int i=0 ; pipes[i].pid != wpid && i<as->localcount ; i++) {
            buf[0] = '\0';
            fgets(buf, BUF_SIZE, pipes[i].err);
            if (strlen(buf) > 0) {
                buf[strcspn(buf, "\n")] = '\0';
                as->local[i]->err = strdup(buf);
                as->local[i]->status = WEXITSTATUS(wstatus);
                ++err;
                break;
            } else {
                buf[0] = '\0';
                fgets(buf, BUF_SIZE, pipes[i].out);
                buf[strcspn(buf, "\n")] = '\0';
                as->local[i]->out = strdup(buf);
                as->local[i]->status = WEXITSTATUS(wstatus);
                break;
            }
        }
    } while (wpid > 0);

    return err;
}

bool file_exists(char *filepath) {
  struct stat   buffer;   
  return (stat (filepath, &buffer) == 0); // if stat works then file exists
}

void send_new_files(char *path, char **rec, int rk, int client) {
    Message *msg;
    char pathbuf[BUF_SIZE] = {0};
    DIR *folder; // holds the dir
    struct dirent *entry; //holds the file
    struct stat st = {0}; //holds file data
    folder = opendir(path);

    while( (entry=readdir(folder)) ) {
        snprintf(pathbuf, BUF_SIZE, "%s/%s", path, entry->d_name); // build path
        if (stat(pathbuf, &st) != -1 && S_ISREG(st.st_mode)) { // if stat succeeds and the object is a file
            bool exists = false;
            for (int i=0 ; i<rk ; i++) { // then needs to be tested against the requirements
                if (strcmp(entry->d_name, rec[i]) == 0){
                    exists = true;
                    break;
                }

            }
            if (!exists) { // if the file was found to not be one of the sent requirements
                printf("%s\n", entry->d_name);
                send_data(client, entry->d_name, CODE_OUT);
                msg = recieve_data(client);
                print_message(msg);
                if (msg->header.type == CODE_OK) {
                    if (send_bfile(client, pathbuf) < 0) // we then want to send it back as it's an obj file
                        exit(error("send_file() failed", EXIT_FAILURE));
                    else
                        printf("%s\n", entry->d_name);
                }
            }
        }
    }
}


void* client_handler(void* c) {
    pthread_detach(pthread_self());


    puts("A client has been connected");

    int client = *((int*)c);
    Message *msg = malloc(sizeof(Message));

    ActionSet *as = malloc(sizeof(char *));
    as = new_set("ACTIONS");

    char *tpath = NULL;
    tpath = new_tmpd();
    printf("OPERATING IN DIR \"%s\"\n", tpath);

    char pathbuff[BUF_SIZE];
    bool connected = true;
    int ncmd;
    char **rec = malloc(sizeof(char *));
    int rk = 0;

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
                ncmd = atoi(msg->data); // includes the count of actions in data
                send_data(client, "AWAITING COMMAND SET", CODE_OK);
                break;
            }

        //  RECIEVE SINGLE COMMAND
            case CODE_ACTION: {
                add_cmd(as, msg->data, NULL);
                break;
            }

        //  RECIEVE REQ COMMAND
            case CODE_REQ: {
                memset(&pathbuff, 0, BUF_SIZE);
                snprintf(pathbuff, BUF_SIZE, "%s/%s", tpath, msg->data);
                if (!file_exists(pathbuff)) {
                    send_data(client, "NEED REQ", CODE_OK);
                    if ((recieve_file(client, pathbuff) < 0)) {
                        send_data(client, "FILE RECIEVE FAILED", CODE_FAIL);
                        connected = 0;
                        break;
                    } else {
                        rec[rk++] = strdup(msg->data);
                        rec = realloc(rec, (rk + 1) * sizeof(char *));
                    }
                } else {
                    send_data(client, "DONT NEED REQ", CODE_BAD);
                    printf("file exists \"%s\"\n", pathbuff);
                }
                break;
            }
        //  CLIENT SIGNALLING ALL ACTIONSET COMMANDS HAVE BEEN SENT
            case CODE_AS_EXECUTE: {
            //  ENSURE WE HAVE RECIEVED ALL ACTION SETS
                if (ncmd == as->localcount) {
                    send_data(client, "RECIEVED, EXECUTING", CODE_OK);
                    int failed;
                    printf("%d\n", failed);
                    if ((failed = exec_set(as, tpath)) > 0) { // execute inside the tmpdir
                        puts("actionset finished executing unsuccessfully");
                        send_data(client, as->local[failed]->err, CODE_ACTION_STATUS + failed);
                    } else {
                        puts("actionset finished executing successfully");

                        send_new_files(tpath, rec, rk, client);
                    }
            //  OTHERWISE ALERT CLIENT
                } else {
                    send_data(client, "INCOMPLETE SET", CODE_FAIL);
                    connected = false;
                }
                print_set(as, 1);
                break;
            }
        }
    }
    memset(&pathbuff, 0, BUF_SIZE);
    snprintf(pathbuff, BUF_SIZE, "rm -r -d %s", tpath);
    //exec_cmd(pathbuff, "./");

    puts("Client Disconnected");
    close(client);
    return NULL;
}
