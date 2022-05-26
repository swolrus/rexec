#include "common.h"

/**
 * allocate a new host
 * @param  host the hostname
 * @param  port the port
 * @return      the new host
 */
Host *new_host(char *host, int port) {
    Host *htmp = NULL;
    htmp = malloc(sizeof(Host *));
    htmp->socket = -1;
    htmp->hostname = strdup(host);
    htmp->port = port;

    return htmp;
}

/**
 * print a host
 */
void print_host(Host *h) {
    printf("Host (%s:%d) -> %d\n", h->hostname, h->port, h->socket);
}

/**
 * print a message (it's header and data)
 */
void print_message(Message *m) {
    if (m->header.length == 0)
        printf("%d\n", m->header.type);
    else
        printf("(%d, %d)%s\n", m->header.type, m->header.length, m->data);
}

/**
 * free a message
 */
void free_message(Message *m) {
    free(m->data);
    free(m);
}

/**
 * connect to a host
 * @param  h the host to connect to, h->socket will be modified to the host fd
 * @return   also returns the host fd
 */
int c_connect(Host *h) {

//  EXIT IF SOCKET ALREADY OPEN
    if (h->socket != -1) {
        return h->socket;
    }

//  CREATE THE ADDRESS
    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(h->port);

    if((h->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        exit(error("socket() failed", EXIT_FAILURE));

//  ALLOW SOCKET TO BE RE-USED (PREVENTS ERRORS STARTING AND STOPPING)
    if (setsockopt(h->socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        exit(error("setsockopt(SO_REUSEADDR) failed", EXIT_FAILURE));

//  CONVERT THE ADDRESS
    if(inet_pton(AF_INET, h->hostname, &serv_addr.sin_addr)<=0)
        exit(error("invalid address", EXIT_FAILURE));

//  CONNECT TO THE ADDRESS
    if (connect(h->socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        exit(error("connect() failed", EXIT_FAILURE));

    printf("New socket connected!\n");

    return h->socket;
}

int h_connect(Host *h) {
    struct sockaddr_in serv_addr;

    if((h->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        exit(error("socket() failed", EXIT_FAILURE));

//  ALLOW SOCKET TO BE RE-USED (PREVENTS ERRORS STARTING AND STOPPING)
    if (setsockopt(h->socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        exit(error("setsockopt(SO_REUSEADDR) failed", EXIT_FAILURE));

//  BUILD THE ADDRESS
    memset(&serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(h->port);

//  BIND SERVER TO THE ADDRESS
    if (bind(h->socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        exit(error("bind() failed", EXIT_FAILURE));

    return h->socket;

}

/**
 * recieve char data
 * @param  socket the socket to read from
 * @return        returns a message struct containing header and data
 */
Message *recieve_data(int socket) {
    Message *msg = malloc(sizeof(Message));
    char buffer[BUF_SIZE] = {0};

//  RECIEVE THE HEADER
    memset(&msg->header, 0, sizeof(HEADER));
    if (recv(socket, &msg->header, sizeof(HEADER), 0) < 0) // recieve length
        exit(error("recv() failed", EXIT_FAILURE));

//  IF DATA RECIEVE THE DATA
    if (msg->header.length != 0) {
        memset(buffer, 0, sizeof(buffer));

        if(recv(socket, &buffer, msg->header.length, 0) < 0) // recieve data
            exit(error("recv() failed", EXIT_FAILURE));
    }
    msg->data = malloc((strlen(buffer) + 1) * sizeof(char));
    msg->data = strdup(buffer);

    return msg;
}

/**
 * send a message in format readable by read_data()
 * uses the struct Message
 * @param socket the socket fd to write
 * @param data   the char data to be written
 * @param mtype  the message type integer to pass in the header
 */
void send_data(int socket, char *data, int mtype) {
    Message *msg = malloc(sizeof(Message));

    msg->header.type = mtype;
    msg->header.length = strlen(data);

//  SEND THE HEADER
    if (send(socket, &msg->header, sizeof(HEADER), 0) < 0) // send length
        exit(error("send() failed", EXIT_FAILURE));

//  IF DATA SEND BYTES
    if (msg->header.length != 0) {
        if (send(socket, data, strlen(data), 0) < 0) // send data
            exit(error("send() failed", EXIT_FAILURE));
    }
}
/**
 * recieve a character file using recieve_data() line by line
 * designed for use with source code as lines over BUF_SIZE will cause stack errors
 * 
 * @param  socket   the socket to send the file through
 * @param  filepath the relative path of the file to write to
 * @return          0 if succes -1 if failed
 */
int recieve_file(int socket, char *filepath) {
    FILE *fp;
    Message *msg;

    fp = fopen(filepath, "wt");
    if (fp == NULL)
        return error("invalid filepath", -1);

    printf("\nrecieving file \"%s\"\n", filepath);
    printf("-----------------------------------------------------\n");
    while (1)
    {
        msg = recieve_data(socket);

        if (msg->header.type == CODE_FILE) {
            fprintf(fp, "%s", msg->data);
            printf("%s", msg->data);
        } else if (msg->header.type == CODE_FILE_END) {
            fclose(fp);
            send_data(socket, "file transfer success", CODE_OK);
            printf("-----------------------------------------------------\n");
            return 0;
        } else {
            fclose(fp);
            send_data(socket, "file transfer failed", CODE_FAIL);
            printf("-----------------------------------------------------\n");
            return error("file transfer failed", -1); 
        }
    }
    fclose(fp);
    printf("-----------------------------------------------------\n");
    return 0;
}

/**
 * recieve a character file using send_data() line by line
 * designed for use with source code as lines over BUF_SIZE will cause stack errors
 * 
 * @param  socket   the socket fd to write to
 * @param  filepath the relative filepath of the file to be sent
 * @return          0 if success -1 if fail
 */
int send_file(int socket, char *filepath) {
    printf("Sending %s\n", filepath);
    FILE *fp;
    Message *msg;

    char buffer[BUF_SIZE] = {0};

    fp = fopen(filepath, "r");

    if (fp == NULL)
        return error("could not open file", -1);

    while (fgets(buffer, BUF_SIZE, fp))
        send_data(socket, buffer, CODE_FILE);

    send_data(socket, filepath, CODE_FILE_END);
    msg = recieve_data(socket);
    print_message(msg);
    fclose(fp);
    return 0;
}

/**
 * Send data as a stream
 * 
 * @param  socket   the sending socket fd
 * @param  filepath the relative path of the target file
 * @return          0 if success -1 if fail
 */
int recieve_bfile(int socket, char *filepath) {
    FILE *fp;
    uint32_t sz;

    fp = fopen(filepath, "wb");
    if (fp == NULL)
        return error("invalid filepath", -1);

    printf("\nrecieving file \"%s\"\n", filepath);


    HEADER header = {0};
    if (recv(socket, &header, sizeof(HEADER), 0) < 0) // recieve length
        exit(error("recv() failed", EXIT_FAILURE));

    if (header.type == CODE_OUT_SZ) {
        sz = header.length;
    } else {
        exit(error("file size not sent", EXIT_FAILURE));
    }

    printf("wanting %d bytes\n", sz);

    // write in binary
    char file_data[BUF_SIZE] = {0};
    
    uint32_t nbytes = 0;
    uint32_t total = 0;
    while (total < sz) {
        size_t rec = sz - total;
        if (rec > sizeof(file_data)) {rec = sizeof(file_data);}

        if ((nbytes = recv(socket, &file_data, rec, 0)) < 0)
            exit(error("recv() bfile data failed", EXIT_FAILURE));
        if (( fwrite(&file_data, sizeof (char), nbytes, fp) ) < 0)
            exit(error("fwrite() failed", EXIT_FAILURE));

        total += nbytes;
        printf("received: %d - %d/%d\n", nbytes, total, sz);

        memset(&file_data, 0, sizeof(file_data));
    }
    Message *msg = NULL;
    msg = recieve_data(socket);
    print_message(msg);

    printf("File '%s' received.\n", filepath);

    fclose(fp);
    return 0;
}

/**
 * send data as a stream
 * 
 * @param  socket   the socket fd
 * @param  filepath the target filepath to read and then send
 * @return          0 if success -1 if failed
 */
int send_bfile(int socket, char *filepath) {
    printf("Sending %s\n", filepath);
    FILE *fp;
    
    char file_data[BUF_SIZE] = {0};

    fp = fopen(filepath, "rb+");

    if (fp == NULL)
        return error("could not open file", -1);

    fseek(fp, 0, SEEK_END);
    uint32_t sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    HEADER header = {0};

//  SEND THE FILE LENGTH
    header.type = CODE_OUT_SZ;
    header.length = sz;
    if (send(socket, &header, sizeof(HEADER), 0) < 0) // send length
        exit(error("send() failed", EXIT_FAILURE));

    // again up to here works

    uint32_t total = 0;
    uint32_t nbytes = 0;
    while ( (nbytes = fread(&file_data, sizeof(char), sizeof(file_data), fp)) > 0 )
    {
        if (( send(socket, &file_data, nbytes, 0)) < 0) // send length
            exit(error("send() failed", EXIT_FAILURE));
        total += nbytes;
        printf("sent: %d - %d/%d\n", nbytes, total, sz);
        memset(file_data, 0, sizeof(file_data));
    }
    send_data(socket, "", CODE_OK);

    printf("File '%s' transfered!\n", filepath);
    fclose(fp);

    return 0;
}

