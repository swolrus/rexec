# A Makefile to build our 'c-server' project

PROJECT =  c-server
HEADERS =  $(PROJECT).h common.h
OBJ     =  main.o socket.o ../common/socket.o ../common/common.o

C99     =  cc -std=c99
CFLAGS  =  -I"../common" -Wall -Werror


$(PROJECT) : $(OBJ)
	$(C99) $(CFLAGS) -o $(PROJECT) $(OBJ) -lm


%.o : %.c $(HEADERS)
	$(C99) $(CFLAGS) -c $<

clean:
	rm -f $(PROJECT) $(OBJ)
