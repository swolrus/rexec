# rexec
remote exec client-server in c

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
./c-client
```
