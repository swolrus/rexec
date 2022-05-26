# REXEC

> Remote exec client-server in c.

## Basic Usage
> This will execute using hardcoded "/testing/rakefile2"
> If you change line 12 in "/client-c/main.c" you can use "../testing/rakefile" and it will compile the program in testing (make sure to `make clean` inside "/testing" so you can confirm it works).
### Server
```bash
cd c-server
make
./c-server '' 6044
```
### Client
```bash
cd c-client
make
./c-client [/path to rakefile]
```



## Run Time Assistance

The client command will store all incoming commands within the same directory as the rakefile is contained in, the client program does not have to be within that folder.

The server will only keep the files sent over for an actionset as long as the connection with the relavent client is alive.

