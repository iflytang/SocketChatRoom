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
    if ((shmid = shmget(IPC_PRIVATE, 1024, PERM)) == -1)
    {
        perror("create shared memory error!");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

int main(int argc, char *argv[])
{
    int sockfd, clientfd; //file descriptor
    int sin_size, recvbytes;
    char buf[MAXLINE] = {0};
    char * read_addr, * write_addr;
    char temp[MAXLINE] = {0};

    pid_t pid, ppid; //定义父子进程标记
//    char *buf, *read_addr, *write_addr, *temp; //需要用到的缓冲区
    struct sockaddr_in their_addr; //定义地址结构

    int shmid;
    shmid = create_shm(); // create shared memory

//    temp = (char *)malloc(255);
    struct sockaddr_in my_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0); //创建基于流套接字
    //bzero(&(my_addr.sin_zero),0);
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET; //IPV4协议族
    my_addr.sin_port = htons(SOCKET_PORT); //转换端口为网络字节序
    my_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&my_addr, sizeof(struct sockaddr)) == -1)
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
        if ((clientfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
        {
            perror("fail to accept");
            exit(1);
        }

        //得到客户端的IP地址输出
        char address[20];
        inet_ntop(AF_INET, &their_addr.sin_addr, address, sizeof(address));

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