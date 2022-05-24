#include "./c-client.h"
#include "../common/strsplit.h"

Message *response;

int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Usage : %s rakefilepath [rakefilepath2 ...]\n", argv[0]);
        //exit(EXIT_FAILURE);
    }

    char *name = "../testing/rakefile2";
    Rake *rake = rake_from_file(name);
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
                exec_local_set(rake->sets[i]);
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


int exec_local_set(ActionSet *as) {

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

//  REMOTE - SEND EACH COMMAND AND THEN REQUIREMENTS SEQUENTIALLY
    char *req;
    for (int i=0 ; i<as->remotecount ; i++) {
        send_data(as->socket, as->remote[i]->cmd, CODE_ACTION);
        if ((req = as->remote[i]->req) != NULL) {
            send_data(as->socket, as->remote[i]->req, CODE_REQ);
            int count;
            char **req;
            req = strsplit(&as->remote[i]->req[8], &count);
            printf("%d", count);
            for (int i=0 ; i<count ; i++) {
                if (send_file(as->socket, req[i]) < 0)
                    exit(error("send_file() failed", EXIT_FAILURE));
            }
            response = recieve_data(as->socket);
            print_message(response);
        } else {
            send_data(as->socket, "none", CODE_REQ);
        }
    }

    send_data(as->socket, "", CODE_AS_END);
//  RECIEVE ACK THAT ACTIONSET COUNT MATCHES (MEANING REMOTE EXEC HAS STARTED)
    response = recieve_data(as->socket);
    print_message(response);

//  WAIT FOR REMOTE COMMANDS TO ALSO EXIT
    response = recieve_data(as->socket);
    print_message(response);

    return 0;
}
