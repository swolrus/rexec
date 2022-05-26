#include "./c-client.h"
#include "../common/common.h"

/**
 * Trims leading whitespace and replaces newline character
 * 
 * @param  lineStart pointer to start of line to be trimmed
 * @return           a pointer to the same string but trimmed
 */
char *trim_line(char *lineStart) {
    while(*lineStart == ' ' || *lineStart == '\t' || *lineStart == '=') {
        lineStart++;
    }
    lineStart[strcspn(lineStart, "\n")] = '\0';
    return lineStart;
}

/**
 * finds port int and truncates param h
 * replaces colon with null pointer leaving original string in tact
 * 
 * @param  h pointer to host of format hostName:00000
 * @return   the integer value of digits following ':'
 */
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

/**
 * allocates memory for the Rake struct
 * @param  fileName the name of the file this rake will represent
 * @return          returns the allocated rake
 */
Rake *new_rake(char *fileName) {
    Rake *rtmp = NULL;
    rtmp = malloc(sizeof(Rake));
    rtmp->fileName = strdup(fileName);
    rtmp->port = 0;
    rtmp->hostCount = 0;
    rtmp->setCount = 0;
    rtmp->hosts = malloc(sizeof(Host *));
    rtmp->sets = malloc(sizeof(ActionSet *));
    return rtmp;
}

/**
 * free a rakes memory
 * @param r target
 */
void free_rake(Rake *r) {
    free(r->fileName);
    for (int i=0 ; i<r->hostCount ; i++) {
        free(r->hosts[i]->hostName);
        free(r->hosts[i]);
    }
    free(r->hosts);
    for (int i=0 ; i<r->setCount ; i++)
        free_set(r->sets[i]);
    free(r);
}

/**
 * print a rake
 * @param r target
 */
void print_rake(Rake *r) {
    int i;

    printf("port -> %d\n", r->port);
    for (i=0 ; i<r->hostCount ; i++) {
        print_host(r->hosts[i]);
    }
    printf("setCount: %d\n", r->setCount);
    printf("\n");
    for (i=0 ; i<r->setCount ; i++) {
        print_set(r->sets[i], 0);
    }
}

/**
 * niaeve file parser constructs rake based on loose rules
 * @param  fileName file to be parsed
 * @return          a new memory location containing the rake
 */
Rake *rake_from_file(char *fileName) {
    char *lineStart;

//  OPEN FILE
    FILE *rfp = fopen(fileName, "r");

    if (rfp == NULL)
        exit(error("could not open file!", EXIT_FAILURE));

    Rake *rtmp = new_rake(fileName);
    char buffer[BUF_SIZE]; // LINE BUFFER

//  PARSE FILE LINES
    while (fgets(buffer, BUF_SIZE, rfp)) {
        lineStart = buffer;
        lineStart = trim_line(lineStart);
    //  FIND PORT LINE
        if (strncmp(lineStart, "PORT", 4) == 0) {
            while (!isdigit(*lineStart)) {
                lineStart++;
            }

            rtmp->port= atoi(lineStart);
    //  FIND HOSTS LINE
        } else if (strncmp(lineStart, "HOSTS", 5)==0) {
            lineStart = strdup(&lineStart[8]);

            while (*lineStart) {
                char *start = lineStart;
                lineStart++;
                while (*lineStart++ != '\0') {
                    if ((*lineStart == ' ')) {
                        *lineStart = '\0';
                    }
                }

                Host *htmp = new_host(start, find_port(start));
                if (htmp->port == -1)
                    htmp->port = rtmp->port;
                rtmp->hosts = realloc(rtmp->hosts, (rtmp->hostCount + 1) * sizeof(*htmp));
                rtmp->hosts[rtmp->hostCount++] = htmp;
            }

    //  FIND ACTIONSET LINE
        } else if (strncmp(lineStart, "actionset", 8) == 0) {

        //  ACTIONSET NUMBER
            lineStart[strcspn(lineStart, ":")] = '\0';
        
        //  PER ACTIONSET TMP VARS
            struct ActionSet *atmp = new_set(lineStart);
            char *previous = NULL;
            char *current = NULL;

        //  FIND COMMANDS UNTIL EMPTY LINE
            while (fgets(buffer, BUF_SIZE, rfp) && strcmp(buffer, "\n") != 0) {
                lineStart = trim_line(buffer);

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
            rtmp->sets = realloc(rtmp->sets, (rtmp->setCount + 1) * sizeof(atmp));
            rtmp->sets[rtmp->setCount++] = atmp;
        }
    }

    print_rake(rtmp);
    fclose(rfp);

    return rtmp;
}
