//---------------------------------客户端client.c---------------------------------------//
#include <stdio.h>
#include <netinet/in.h> //定义数据结构sockaddr_in
#include <sys/socket.h> //定义socket函数以及数据结构
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>

#define SOCKET_PORT  2017
#define MAXLINE 1024

int main(int argc, char *argv[])
{
    struct sockaddr_in6 client;
    pid_t pid;
    int clientfd, sendbytes, recvbytes;
    char * client_addr = "::1";
    char * client_name = "rick";
    char buf[MAXLINE] = {0};

    struct hostent *host; //主机信息数据结构
    char *buf_read;

    // read the server_addr and server_name from **agrv
    if (argc != 3) {
        perror("wrong input!\nusage:\n\t./x.o <IPv6 Address> <String name>\n");
        exit(EXIT_FAILURE);
    } else {
        client_addr = argv[1];
        client_name = argv[2];
    }


    if ((clientfd = socket(AF_INET6, SOCK_STREAM, 0)) == -1)
    {
        perror("fail to create socket");
        exit(1);
    }
    bzero(&client, sizeof(client));
    client.sin6_family = AF_INET6;
    client.sin6_port = htons(SOCKET_PORT);
    inet_pton(AF_INET6, client_addr, &client.sin6_addr);
    //客户端连接服务端
    if (connect(clientfd, (struct sockaddr *)&client, sizeof(client)) == -1)
    {
        perror("fail to connect");
        exit(1);
    }
//    buf = (char *)malloc(120);
    memset(buf, 0, 120);
    buf_read = (char *)malloc(100);

    if (recv(clientfd, buf, 100, 0) == -1)
    {
        perror("fail to recv");
        exit(1);
    }
    printf("\n%s\n", buf);
    pid = fork();
    while (1)
    {
        if (pid > 0)
        {
            //父进程发送消息
            strcpy(buf, argv[2]);
            strcat(buf, ":");
            memset(buf_read, 0, 100);
            fgets(buf_read, 100, stdin);
            strncat(buf, buf_read, strlen(buf_read) - 1);
            if ((sendbytes = send(clientfd, buf, strlen(buf), 0)) == -1)
            {
                perror("fail to send");
                exit(1);
            }
        }
        else if (pid == 0)
        {
            //子进程接受消息
            memset(buf, 0, 100);
            if (recv(clientfd, buf, 100, 0) <= 0)
            {
                perror("fail to recv");
                close(clientfd);
                raise(SIGSTOP);
                exit(1);
            }
            printf("%s\n", buf);
        }
        else
            perror("fork error");
    }
    close(clientfd);
    return 0;
}