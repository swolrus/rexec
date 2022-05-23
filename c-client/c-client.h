#ifndef C_CLIENT_H_
#define C_CLIENT_H_

#include <sys/wait.h>
#include <sys/types.h>

#include "../common/common.h"

// GLOBAL FUNCTIONS
// main.c
extern char *           trim_line(char *);
extern Rake *           rake_from_file(char *);
extern int              exec_local_set(ActionSet *);

// rake.c
extern Rake *           new_rake(char *);
extern void             free_rake(Rake *);
extern void             print_rake(Rake *);
extern ActionSet *      new_set(char *);
extern void             free_set(ActionSet *);
extern void             print_set(ActionSet *);

#endif // C_CLIENT_H_
