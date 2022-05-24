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

void print_rake(Rake *r) {
    int i;

    printf("port -> %d\n", r->port);
    for (i=0 ; i<r->hostcount ; i++) {
        print_host(r->hosts[i]);
    }
    printf("setcount: %d\n", r->setcount);
    printf("\n");
    for (i=0 ; i<r->setcount ; i++) {
        print_set(r->sets[i], 0);
    }
}

Rake *rake_from_file(char *filename) {
    char *linestart;

//  OPEN FILE
    FILE *rfp = fopen(filename, "r");

    if (rfp == NULL)
        exit(error("could not open file!", EXIT_FAILURE));

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
                    add_cmd(atmp, previous, current);
                    previous = NULL;
                } else {

                // REQUIREMENTS ARE NULL AND PROGRESS ONE LINE
                    add_cmd(atmp, previous, NULL);
                    previous = strdup(current);
                }
            }
            if (previous != NULL)
                add_cmd(atmp, current, NULL);

        //  ASSIGN THE ACTIONSET
            rtmp->sets = realloc(rtmp->sets, (rtmp->setcount + 1) * sizeof(atmp));
            rtmp->sets[rtmp->setcount++] = atmp;
        }
    }

    print_rake(rtmp);
    fclose(rfp);

    return rtmp;
}
