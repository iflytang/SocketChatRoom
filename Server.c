/**
 * Created by tsf on 17-11-15.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define MAXLINE 1024
#define TRUE    1
#define SOCKETPORT 2018
#define PENDINGQUEUE 10

void quit(int signal) {
//    printf("server@eeis:~$ Socket teardown");
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    int sockfd;   // a socket file descriptor
    char recv_buf[MAXLINE + 1] = {0};
    char send_buf[MAXLINE + 1] = {0};
    char* server_addr = "::1";  // default ipv6 address
    struct sockaddr_in6 server, client;

    // get the server_addr from input argv
    if(argc != 2) {
        perror("server@eeis:~$ wrong input!\nusage: \n\tx.o <IPv6 Address>");
    } else {
        server_addr = argv[1];
        printf("server@eeis:~$ Server ipv6: %s\n", server_addr);
    }

    // login time
    time_t login_time;
    login_time = time(&login_time);
    char* src = ctime(&login_time);
    char dst[100];
    strncpy(dst, src, strlen(src) - 1);
    dst[strlen(src) - 1] = '\0';
    printf("server@eeis:~$ Login time: %s from %s\n", dst, server_addr);  // print time as human readable format

    // create a socket
    sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("server@eeis:~$ make_tcp_connection:: socket");
        exit(EXIT_FAILURE);
    }

    // give the socket a name
    bzero(&server, sizeof(server));
    server.sin6_family = AF_INET6;
    server.sin6_port = htons(SOCKETPORT);
    inet_pton(AF_INET6, server_addr, &server.sin6_addr);

    // bind socket
    int bind_socket = bind(sockfd, (struct sockaddr *) &server, sizeof(server));
    if(bind_socket < 0) {
        perror("server@eeis:~$ make_tcp_socket:: bind");
        exit(EXIT_FAILURE);
    }

    // listen socket
    int listen_socket = listen(sockfd, PENDINGQUEUE);  // making it as sever socket, argument n specifies, the length of the queue for pending connections
    if(listen_socket < 0) {
        perror("server@eeis:~$ make_tcp_socket:: listen");
        exit(EXIT_FAILURE);
    }

    // accept client socket request
    printf("server@eeis:~$ Waiting clients ...\n");
    socklen_t client_len =  sizeof(struct sockaddr);
    int fd = accept(sockfd, (struct sockaddr *) &client, &client_len);
    if(fd < 0) {
        perror("server@eeis:~$ make_tcp_socket:: accept");
        exit(EXIT_FAILURE);
    } else {
        printf("server@eeis:~$ Accepted! Socket has been established.\n");
    }

    // advice to input
//    printf("server@eeis:~$ Happy to chat.");

    // create a new progress
    pid_t pid = fork();   // if return value is 0, then it's child process
    if(pid < 0) {
        perror("server@eeis:~$ create_new_program:: fork");
    }

    // child process with pid == 0, send data to client; otherwise, receive data from client
    if(pid != 0) {
        do {
            int recv_len = read(fd, recv_buf, MAXLINE);
            if (recv_len < 0) {
                perror("server@eeis:~$ Wrong Read.\n");
                break;
            } else if (recv_len == 0) {
                printf("server@eeis:~$ Client closed!\n");
                break;
            }
            recv_buf[recv_len] = '\0';
//            printf("Server recv_buf is %s", recv_buf);
//            printf("strcmp(recv_buf, END) = ? %d.", strcmp(recv_buf, "END"));


            if (strcmp(recv_buf, "END") != 10) {
                if (fputs(recv_buf, stdout) == EOF) {
                    perror("server@eeis:~$ fputs error.\n");
                }
            } else {
                printf("server@eeis:~$ Receive END!\n");
                break;   // receive "END"
            }
        }while (TRUE);  // when receive "END", stop
        kill(pid, SIGUSR1);  // kill child progress
    } else {
        signal(SIGUSR1, quit);
        while(fgets(send_buf, MAXLINE, stdin) != NULL) {
            send(fd, send_buf, strlen(send_buf), 0);   // send data to fd
            memset(send_buf, 0, strlen(send_buf));     // clear the buffer
        }
    }
    close(fd);
    close(sockfd);
    printf("server@eeis:~$ Socket connection teardowns.\n");

    return 0;
}
