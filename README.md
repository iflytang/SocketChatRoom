# SocketChatRoom
Linux c to implement chat room with shared memory using socket.

In the project, we use the Shared Memory to implement chat room. The valid source codes are Server_v2.c and Client_v2.c .

There can be divided into three types of socket communication: signal communication, dual communication, multiple communication.
They are based on IPv6.

All of them should be compile first.
```apple js
gcc -o xxx xxx.c
```
## 1. TCPServer.c & TCPClient.c
These two files implements single direction communication.
Server RECEIVE data, Client SEND data.
```apple js
./x.out ::1
```
## 2. Server.c & Client.c
These two files implements dual direction communication.
Both of Server and Client can SEND and RECEIVE data. 
Close session by entering "END" by any one.
```apple js
./x.out ::1
```
## 3. Server_v2.c & Client_v2.c
These two files implements multiple direction communication, or chatting room based on SHARED MEMORY.
Server RECEIVE and FORWARD data to all client.

You should input two argument when you run.
```apple js
./x.out IPv6_address your_name
// for example
./x.out ::1 iflytang
```
For Server, it acts as a monitor. Server receive data sent from one client, then store them in SHARED MEMORY, and forward to other client.
We can see online people in Server console.

For Client, it acts as a user. When some one send "END", all socket will be closed.

