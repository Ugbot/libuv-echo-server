# libuv-echo-server

Tested with valgrind and flood udp script. Memory leaks are not detected

## Build & Run 
```
mkdir build && cd build
cmake ../
make
./echo_server
```

for testing can be used command:
```
nc -u <IP> <PORT>
```