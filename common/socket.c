#include "common.h"

Host *new_host(char *host, int port) {
    Host *htmp = NULL;
    htmp = malloc(sizeof(Host *));
    htmp->socket = -1;
    htmp->hostname = strdup(host);
    htmp->port = port;

    return htmp;
}

void print_host(Host *h) {
    printf("Host (%s:%d) -> %d\n", h->hostname, h->port, h->socket);
}

void print_message(Message *m) {
    if (m->header.length == 0)
        printf("%d\n", m->header.type);
    else
        printf("(%d, %d)%s\n", m->header.type, m->header.length, m->data);
}

void free_message(Message *m) {
    free(m->data);
    free(m);
}

int c_connect(Host *h) {

    if (h->socket != -1) {
        return h->socket;
    }

    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(h->port);

    if((h->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        exit(error("socket() failed", EXIT_FAILURE));

    if (setsockopt(h->socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        exit(error("setsockopt(SO_REUSEADDR) failed", EXIT_FAILURE));

    if(inet_pton(AF_INET, h->hostname, &serv_addr.sin_addr)<=0)
        exit(error("invalid address", EXIT_FAILURE));

    if (connect(h->socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        exit(error("connect() failed", EXIT_FAILURE));

    printf("New socket connected!\n");

    return h->socket;
}

int h_connect(Host *h) {
    struct sockaddr_in serv_addr;

    if((h->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        exit(error("socket() failed", EXIT_FAILURE));

    if (setsockopt(h->socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        exit(error("setsockopt(SO_REUSEADDR) failed", EXIT_FAILURE));

    memset(&serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(h->port);

    if (bind(h->socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        exit(error("bind() failed", EXIT_FAILURE));

    return h->socket;

}

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

int recieve_bfile(int socket, char *filepath) {
    FILE *fp;

    char buf[BUF_SIZE] = {0};

    fp = fopen(filepath, "wb+");
    if (fp == NULL)
        return error("invalid filepath", -1);

    printf("\nrecieving file \"%s\"\n", filepath);

    HEADER header = {0};
    if (recv(socket, &header, sizeof(HEADER), 0) < 0) // recieve length
        exit(error("recv() failed", EXIT_FAILURE));
    if (header.type == CODE_FILE) {
        uint32_t total = 0;
        if(recv(socket, &total, header.length, 0) < 0) // the files length into variable total
            exit(error("recv() failed", EXIT_FAILURE));
        printf("TOTOAL: %d\n", total);
        // write in binary
        int length = 0;
        while ((length = recv(socket, buf, BUF_SIZE, 0)) && total > 0) {
            fwrite(buf, sizeof (char), length, fp);
            if (strlen(buf) > 0) {
                printf("received: %d\n", (int)strlen(buf));
                total -= length;
                memset(&buf, 0, sizeof (buf));
            }
        }
        printf("File '%s' received.\n", filepath);
    }
    fclose(fp);
    return 0;
}

int send_bfile(int socket, char *filepath) {
    printf("Sending %s\n", filepath);
    FILE *fp;
    
    char buf[BUF_SIZE] = {0};

    fp = fopen(filepath, "rb+");

    if (fp == NULL)
        return error("could not open file", -1);

    fseek(fp, 0L, SEEK_END);
    uint32_t sz = ftell(fp);
    
    HEADER header = {0};

    header.type = CODE_FILE;
    header.length = sizeof(uint32_t);
    if (send(socket, &header, sizeof(HEADER), 0) < 0) // send length
        exit(error("send() failed", EXIT_FAILURE));
    if (send(socket, &sz, sizeof(sz), 0) < 0) // send data
        exit(error("send() failed", EXIT_FAILURE));


    int length = 0, total = 0;
    while ((length = fread(buf, sizeof (char), BUF_SIZE, fp)) > 0) {
        printf("Length : '%d'\n", length);
        send(socket, buf, length, 0);
        memset(&buf, 0, BUF_SIZE);
        total += length;
    }
    printf("File '%s' transfered!\n", filepath);
    fclose(fp);

    return 0;
}

