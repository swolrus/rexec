#ifndef C_SERVER_H_
#define C_SERVER_H_

#include <pthread.h>

#include "../common/common.h"

extern int              exec_set(ActionSet *, char *); // execute a as
extern bool             file_exists(char *);
extern void*            client_handler(void*); // deals with logic flow after accepting a client
extern void             send_new_files(char *, char **, int, int);

#endif  // C_SERVER_H_
