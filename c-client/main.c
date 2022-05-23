#include "./c-client.h"

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
                error("fork() failed");
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
    for (int i=0 ; i<as->localcount ; i+=2) {
        if ((child_pid = fork()) == 0) {
            if ((status = system(as->local[i])) != -1){
                //printf("%s // EXITED (%d)\n", as->local[i], WEXITSTATUS(status));
                exit(EXIT_SUCCESS);
            }
            error("system() failed");
        }
    }

//  CONFIRM START OF ACTIONSET
    char a[10];
    sprintf(a, "%d", as->remotecount / 2);
    send_data(as->socket, a, CODE_AS_START);
    response = recieve_data(as->socket);
    print_message(response);

//  REMOTE - SEND EACH COMMAND AND THEN REQUIREMENTS SEQUENTIALLY
    for (int i=0 ; i<as->remotecount ; i+=2) {
        send_data(as->socket, as->remote[i], CODE_ACTION);
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
