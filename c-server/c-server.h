#ifndef C_SERVER_H_
#define C_SERVER_H_

#include <pthread.h>

#include "../common/common.h"

extern void*            client_handler(void*);
extern int              h_connect(Host *);
extern Message          *recieve_data(int);
extern void             send_data(int, char *, int);
extern void             print_message(Message *);

#endif  // C_SERVER_H_
