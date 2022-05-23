PROJECT 	= rexec
SRCS 		= c-client c-server common
BUILDDIR 	= build
OBJ 		= $(SRCS:%/%.c=$(BUILDDIR)/%.o)
HEADERS 	= $(SRCS:%/%.h)

C99     =  cc -std=c99
CFLAGS  =  -Wall -Werror

$(PROJECT) : $(OBJ)
    $(C99) $(CFLAGS) -o $@ $^

%.o : %.c $(HEADERS)
    $(C99) $(CFLAGS) -c $<

clean:
    rm -f $(PROJECT) $(OBJ) 
