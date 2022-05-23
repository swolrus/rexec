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

int c_connect(Host *h) {

    if (h->socket != -1) {
        return h->socket;
    }

    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(h->port);

    if((h->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error("socket() failed");

    if (setsockopt(h->socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        error("setsockopt(SO_REUSEADDR) failed");

    if(inet_pton(AF_INET, h->hostname, &serv_addr.sin_addr)<=0)
        error("Error : Invalid address");

    if (connect(h->socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("connect() failed");

    printf("New socket connected!\n");

    return h->socket;
}

int h_connect(Host *h) {
    struct sockaddr_in serv_addr;

    if((h->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error("socket() failed");

    if (setsockopt(h->socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        error("setsockopt(SO_REUSEADDR) failed");

    memset(&serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(h->port);

    if (bind(h->socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("bind() failed");

    return h->socket;

}

Message *recieve_data(int socket) {
    Message *msg = malloc(sizeof(Message));
    char buffer[BUF_SIZE];

//  RECIEVE THE HEADER
    memset(&msg->header, 0, sizeof(HEADER));
    if (recv(socket, &msg->header, sizeof(HEADER), 0) < 0) // recieve length
        error("read() failed");

//  IF DATA RECIEVE THE DATA
    if (msg->header.length != 0) {
        memset(buffer, 0, sizeof(buffer));

        if( recv(socket, &buffer, msg->header.length, 0) < 0) // recieve data
            error("read() failed");
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
        error("write() failed");

//  IF DATA SEND BYTES
    if (msg->header.length != 0) {
        if (send(socket, data, strlen(data), 0) < 0) // send data
            error("write() failed");
    }
}
