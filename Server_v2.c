/**
* Created by tsf on 17-11-18.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <errno.h>
#include <sys/shm.h>
#include <time.h>
#include <arpa/inet.h>

#define PERM S_IRUSR | S_IWUSR
#define SOCKET_PORT  2017
#define PENDING_QUEUE 15
#define MAXLINE 1024
#define TRUE 1
#define WELCOME "|================ Chatting Room ===============|"


// when interrupted, call this method
void quit(int signal) {
    printf("> Socket teardowns!\n");
    exit(EXIT_SUCCESS);
}

// get current time
char * get_cur_time()
{
    time_t current_time;
    current_time = time(&current_time);
    char * now = ctime(&current_time);

    return now;
}

// create shared memory
int create_shm()
{
    int shmid;
    if ((shmid = shmget(IPC_PRIVATE, MAXLINE, PERM)) == -1)
    {
        perror("> create shared memory error!");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

int main(int argc, char *argv[])
{
    int sockfd, clientfd; //file descriptor
    char buf[MAXLINE] = {0};
    char temp[MAXLINE] = {0};
    struct  sockaddr_in6 server, client;
    char * server_addr = "::1"; // server's default ipv6 address
    char * server_name = "tang"; // server's name when in communication

    // create shared memory
    int shmid;
    shmid = create_shm();

    // read the server_addr and server_name from **agrv
    if (argc != 3) {
        perror("> wrong input!\nusage:\n\t./x.o <IPv6 Address> <String name>\n");
        exit(EXIT_FAILURE);
    } else {
        server_addr = argv[1];
        server_name = argv[2];
    }

    // display Chatting room monitor information
    printf("|=============== Chatting room monitor ===============|\n");

    // print log_in information
    char * current_time = get_cur_time();     // get the current system time
    printf("> monitor %s login from %s at %s", server_name, server_addr, current_time);

    // create and check a server socket
    sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("> make_tcp_connection:: socket");
        exit(EXIT_FAILURE);
    }

    // give the socket a name
    bzero(&server, sizeof(server));
    server.sin6_family = AF_INET6;
    server.sin6_port = htons(SOCKET_PORT);
    inet_pton(AF_INET6, server_addr, &server.sin6_addr);  // assign argv[1]:ipv6 to server_addr

    // bind socket
    int bind_socket = bind(sockfd, (struct sockaddr *) &server, sizeof(server));
    if(bind_socket < 0) {
        perror("> make_tcp_connection:: bind");
        exit(EXIT_FAILURE);
    }

    // listen socket
    int listen_socket = listen(sockfd, PENDING_QUEUE);  // making it as server socket
    if(listen_socket < 0) {
        perror("> make_tcp_connection:: listen");
        exit(EXIT_FAILURE);
    }
    printf("> listening...\n");

    char * read_addr, * write_addr;
    pid_t pid, ppid;
    int online_people = 0;     // indicate how many people online
    while (TRUE)
    {
        // accept a client socket
        socklen_t client_len = sizeof(struct sockaddr);
        clientfd = accept(sockfd, (struct sockaddr *) &client, &client_len);
        if(clientfd < 0) {
            perror("> make_tcp_connection:: accept");
            exit(EXIT_FAILURE);
        }

        // display online people on monitor
        printf("> accepted. online numbers: %d\n", ++online_people);
        char online[255] = "online ID: ";
        char str[255] = {0};
        sprintf(str, "%d", online_people);   // convert online_people into chars
        strcat(online, str);

        // send WELCOME and online ID to client
        send(clientfd, WELCOME, strlen(WELCOME), 0);
        send(clientfd, online, strlen(online), 0);

        // clear
        bzero(online, strlen(online));
        strcpy(online, "online ID: ");
        bzero(str, strlen(str));

        pid = fork(); // create a new child progress with return value ZERO
        // pid == 0, RECEIVE DATA and WRITE (ppid > 0) to SM, then SEND (ppid == 0) to all client
        if (pid == 0) {
            ppid = fork();  // create new child progress of pid
            while (TRUE)
            {
                if (ppid > 0)
                {
                    // clear buf
                    bzero(buf, MAXLINE);

                    // receive data
                    int recv_len = read(clientfd, buf, MAXLINE);
                    if(recv_len <= 0) {
                        perror("> wrong receive from socket.\n");
                        close(clientfd);
                        kill(ppid, SIGUSR1);  // kill its child process, permit read data from SM to send
                        kill(getppid(), SIGUSR1); // kill its father process, permit send data to SM to write
                        exit(EXIT_FAILURE);
                    }

                    write_addr = shmat(shmid, 0, 0); //shmat将shmid所代表的全局的共享存储区关联到本进程的进程空间
                    memset(write_addr, '\0', 1024);

                    //把接收到的消息存入共享存储区中
                    strncpy(write_addr, buf, 1024);

                    //把接收到的消息连接此刻的时间字符串输出到标准输出
                    char * now = get_cur_time();
                    strcat(buf, now);
                    printf("%s\n", buf);
                }
                else if (ppid == 0)
                {
                    //子进程用于发送消息
                    signal(SIGUSR1, quit);
                    sleep(1); //子进程先等待父进程把接收到的信息存入共享存储区
                    read_addr = shmat(shmid, 0, 0); //读取共享存储区的内容

                    //temp存储上次读取过的内容,每次先判断是否已经读取过该消息
                    if (strcmp(temp, read_addr) != 0)
                    {
                        strcpy(temp, read_addr); //更新temp，表示已经读取过该消息

                        char * now = get_cur_time();
                        strcat(read_addr, now);
                        if (send(clientfd, read_addr, strlen(read_addr), 0) == -1)
                        {
                            perror("fail to send");
                            exit(1);
                        }
                        memset(read_addr, '\0', 1024);
                        strcpy(read_addr, temp);
                    }
                }
                else
                    perror("fai to fork");
            }
        } else {
            // kill parent progress
            signal(SIGUSR1, quit);
        }
    }
    printf("------------------------------------\n");
//    free(buf);
    close(sockfd);
    close(clientfd);
    return 0;
}