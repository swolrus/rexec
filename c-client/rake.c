#include "./c-client.h"
#include "../common/common.h"


char *trim_line(char *linestart) {
    while(*linestart == ' ' || *linestart == '\t' || *linestart == '=') {
        linestart++;
    }
    linestart[strcspn(linestart, "\n")] = '\0';
    return linestart;
}


int find_port(char * h) {
    char *port = h;
    while (*port != ':') {
        if (*port == '\0')
            return -1;
        port++;
    }
    *port++ = '\0';
    return atoi(port);
}

ActionSet *new_set(char *name) {
    ActionSet *atmp = NULL;
    atmp = malloc(sizeof(ActionSet));
    atmp->name = strdup(name);
    atmp->localcount = 0;
    atmp->remotecount = 0;
    atmp->local = malloc(2 * sizeof(char *));
    atmp->remote = malloc(2 * sizeof(char *));
    return atmp;
}

Rake *new_rake(char *filename) {
    Rake *rtmp = NULL;
    rtmp = malloc(sizeof(Rake));
    rtmp->filename = strdup(filename);
    rtmp->port = 0;
    rtmp->hostcount = 0;
    rtmp->setcount = 0;
    rtmp->hosts = malloc(sizeof(Host *));
    rtmp->sets = malloc(sizeof(ActionSet *));
    return rtmp;
}

void free_set(ActionSet *as) {
    free(as->name);
    free(as->local);
    free(as->remote);
    free(as);
}

void free_rake(Rake *r) {
    free(r->filename);
    for (int i=0 ; i<r->hostcount ; i++) {
        free(r->hosts[i]->hostname);
        free(r->hosts[i]);
    }
    free(r->hosts);
    for (int i=0 ; i<r->setcount ; i++)
        free_set(r->sets[i]);
    free(r);
}

void print_set(ActionSet *as) {
    int i;
    printf("\n[ %s ]\n", as->name);
    printf("Local:\n");
    for (i=0 ; i<as->localcount ; i++) {
        if (as->local[i] == NULL) {
            printf("  - no requirements\n");
        } else {
            if (i % 2 == 1) {
                printf("  - %s\n", as->local[i]);
            } else {
                printf("- %s\n", as->local[i]);
            }
        }
    }
    printf("Remote:\n");
    for (i=0 ; i<as->remotecount ; i++) {
        if (as->remote[i] == NULL) {
            printf("  - no requirements\n");
        } else {
            if (i % 2 == 1) {
                printf("  - %s\n", as->remote[i]);
            } else {
                printf("- %s\n", as->remote[i]);
            }
        }
    }
}

void print_rake(Rake *r) {
    int i;

    printf("port -> %d\n", r->port);
    for (i=0 ; i<r->hostcount ; i++) {
        print_host(r->hosts[i]);
    }
    printf("setcount: %d\n", r->setcount);
    printf("\n");
    for (i=0 ; i<r->setcount ; i++) {
        print_set(r->sets[i]);
    }
}

void new_command(ActionSet *as, char *command, char *requires) {
//  SETUP CMD AND REQ STRINGS
    int remote = 0; // 0 = local, 1 = remote

    if (!strncmp(command, "remote-", 6)) {
    // TRUNCATE REMOTE PREFIX
        remote = 1; // is remote
        command = &command[7];
    }
    char *cmd = malloc((strlen(command) + 1) * sizeof(char));
    snprintf(cmd, (strlen(command) + 1) * sizeof(char), "%s", command);

//  STORE REQ (OR LEAVE NULL IF NONE)
    char *req = NULL;
    if (requires != NULL) {
        req = malloc((strlen(requires) + 1) * sizeof(char));
        req = strdup(requires);
    }

//  STORE COMMAND AND REQ IN APPROPRIATE ARRAY
    if (remote == 1) {
    //  AS REMOTE
        as->remote = realloc(as->remote, (as->remotecount + 2) * sizeof(char*));
        as->remote[as->remotecount++] = cmd;
        as->remote[as->remotecount++] = req;
    } else {
    //  AS LOCAL
        as->local = realloc(as->local, (as->localcount + 2) * sizeof(char*));
        as->local[as->localcount++] = cmd;
        as->local[as->localcount++] = req;
    }
    
}

Rake *rake_from_file(char *filename) {
    char *linestart;

//  OPEN FILE
    FILE *rfp = fopen(filename, "r");

    if (rfp == NULL) {
        perror("Error : could not open file!");
        exit(EXIT_FAILURE);
    }

    Rake *rtmp = new_rake(filename);
    char buffer[BUF_SIZE]; // LINE BUFFER

//  PARSE FILE LINES
    while (fgets(buffer, BUF_SIZE, rfp)) {
        linestart = buffer;
        linestart = trim_line(linestart);
    //  FIND PORT LINE
        if (strncmp(linestart, "PORT", 4) == 0) {
            while (!isdigit(*linestart)) {
                linestart++;
            }

            rtmp->port= atoi(linestart);
    //  FIND HOSTS LINE
        } else if (strncmp(linestart, "HOSTS", 5)==0) {
            linestart = strdup(&linestart[8]);

            while (*linestart) {
                char *start = linestart;
                linestart++;
                while (*linestart++ != '\0') {
                    if ((*linestart == ' ')) {
                        *linestart = '\0';
                    }
                }

                Host *htmp = new_host(start, find_port(start));
                if (htmp->port == -1)
                    htmp->port = rtmp->port;
                rtmp->hosts = realloc(rtmp->hosts, (rtmp->hostcount + 1) * sizeof(*htmp));
                rtmp->hosts[rtmp->hostcount++] = htmp;
            }

    //  FIND ACTIONSET LINE
        } else if (strncmp(linestart, "actionset", 8) == 0) {

        //  ACTIONSET NUMBER
            linestart[strcspn(linestart, ":")] = '\0';
        
        //  PER ACTIONSET TMP VARS
            struct ActionSet *atmp = new_set(linestart);
            char *previous = NULL;
            char *current = NULL;

        //  FIND COMMANDS UNTIL EMPTY LINE
            while (fgets(buffer, BUF_SIZE, rfp) && strcmp(buffer, "\n") != 0) {
                linestart = trim_line(buffer);

            //  SKIP LINE IF FIRST ITER OR STORED TWO LINES
                if (previous == NULL) {
                    previous = strdup(buffer);
                    previous = trim_line(previous);
                    fgets(buffer, BUF_SIZE, rfp);
                }

                current = strdup(buffer);
                current = trim_line(current);

                if (!strncmp(current, "requires", 7)) {

                //  SAVE REQUIREMENTS AND PROGRESS TWO LINES
                    new_command(atmp, previous, current);
                    previous = NULL;
                } else {

                // REQUIREMENTS ARE NULL AND PROGRESS ONE LINE
                    new_command(atmp, previous, NULL);
                    previous = strdup(current);
                }
            }
            if (previous != NULL)
                new_command(atmp, current, NULL);

        //  ASSIGN THE ACTIONSET
            rtmp->sets = realloc(rtmp->sets, (rtmp->setcount + 1) * sizeof(atmp));
            rtmp->sets[rtmp->setcount++] = atmp;
        }
    }

    print_rake(rtmp);
    fclose(rfp);

    return rtmp;
}
