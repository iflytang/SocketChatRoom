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
#define WELCOME "|================ Chatting Room ===============|"


// when interrupted, call this method
void quit(int signal) {
    printf("Socket teardowns!\n");
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
        perror("create shared memory error!");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

int main(int argc, char *argv[])
{
    int sockfd, clientfd; //file descriptor
    int  recvbytes;
    char buf[MAXLINE] = {0};
    char * read_addr, * write_addr;
    char temp[MAXLINE] = {0};
    struct  sockaddr_in6 server, client;
    char * server_addr = "::1"; // server's default ipv6 address
    char * server_name = "tang"; // server's name when in communication

    pid_t pid, ppid;

    // create shared memory
    int shmid;
    shmid = create_shm();

    // read the server_addr and server_name from **agrv
    if (argc != 3) {
        perror("wrong input!\nusage:\n\t./x.o <IPv6 Address> <String name>\n");
        exit(EXIT_FAILURE);
    } else {
        server_addr = argv[1];
        server_name = argv[2];
    }

//    struct sockaddr_in my_addr;
    sockfd = socket(AF_INET6, SOCK_STREAM, 0); //创建基于流套接字
    // give the socket a name
    bzero(&server, sizeof(server));
    server.sin6_family = AF_INET6;
    server.sin6_port = htons(SOCKET_PORT);
    inet_pton(AF_INET6, server_addr, &server.sin6_addr);

    if (bind(sockfd, (struct sockaddr*) &server, sizeof(server)) == -1)
    {
        perror("fail to bind");
        exit(1);
    }

    printf("bind success!\n");

    char * now = get_cur_time();
    printf("Time is : %s\n", now);

    if (listen(sockfd, PENDING_QUEUE) == -1)
    {
        //在指定端口上监听
        perror("fail to listen");
        exit(1);
    }

    printf("listen....\n");
    while (1)
    {
        /*
        if (listen(sockfd, BACKLOG) == -1)
        {
            perror("fail to listen");
            exit(1);
        }
        */
        //接受一个客户端的连接请求
        socklen_t client_len = sizeof(struct sockaddr);
        if ((clientfd = accept(sockfd, (struct sockaddr *)&client, &client_len)) == -1)
        {
            perror("fail to accept");
            exit(1);
        }

        //得到客户端的IP地址输出
        char address[20];
        inet_ntop(AF_INET, &client.sin6_addr, address, sizeof(address));

        printf("accept from %s\n", address);
        send(clientfd, WELCOME, strlen(WELCOME), 0); //发送问候信息
//        buf = (char *)malloc(255);

        ppid = fork(); //创建子进程
        if (ppid == 0) //子进程
        {
            pid = fork(); //子进程创建子进程
            while (1)
            {
                if (pid > 0)
                {
                    //buf = (char *)malloc(255);
                    //父进程用于接收信息
                    memset(buf, 0, 255);
                    printf("OK\n");
                    if ((recvbytes = recv(clientfd, buf, 255, 0)) <= 0)
                    {
                        perror("fail to recv");
                        close(clientfd);
                        raise(SIGKILL);
                        exit(1);
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
                else if (pid == 0)
                {
                    //子进程用于发送消息
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
        }
    }
    printf("------------------------------------\n");
//    free(buf);
    close(sockfd);
    close(clientfd);
    return 0;
}