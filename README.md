# SocketChatRoom
Linux c to implement chat room with shared memory using socket.

In the project, we use the Shared Memory to implement chat room. The valid source codes are Server_v2.c and Client_v2.c .

## 1. compile
It is easy to compile source code with command : 
```
gcc -o Server_v2.c
gcc -o Client_v2.c
```
## 2. start
You should input two argument when you run.
```apple js
./x.out IPv6_address your_name
```
For Server, it acts as a monitor. Server receive data sent from one client, then store them in SHARED MEMORY, and forward to other client.
We can see online people in Server console.

For Client, it acts as a user. When some one send "END", all socket will be closed.

