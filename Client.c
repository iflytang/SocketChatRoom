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
//    printf("client@eeis:~$ Socket teardown");
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    int sockfd;   // a socket file descriptor
    char recv_buf[MAXLINE + 1] = {0};
    char send_buf[MAXLINE + 1] = {0};
    char *client_addr = "::1";  // default ipv6 address
    struct sockaddr_in6 server;

    // get the server_addr from input argv
    if (argc != 2) {
        perror("server@eeis:~$ wrong input!\nusage: \n\tx.o <IPv6 Address>");
    } else {
        client_addr = argv[1];
        printf("server@eeis:~$ Client ipv6: %s\n", client_addr);
    }

    // login time
    time_t login_time;
    login_time = time(&login_time);
    char *src = ctime(&login_time);
    char dst[100];
    strncpy(dst, src, strlen(src) - 1);
    dst[strlen(src) - 1] = '\0';
    printf("client@eeis:~$ Login time: %s from %s\n", dst, client_addr);  // print time as human readable format

    // create a socket
    sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("client@eeis:~$ make_tcp_connection:: socket");
        exit(EXIT_FAILURE);
    }

    // give the socket a name
    bzero(&server, sizeof(server));
    server.sin6_family = AF_INET6;
    server.sin6_port = htons(SOCKETPORT);
    inet_pton(AF_INET6, client_addr, &server.sin6_addr);

    // connect the server socket
    int connect_socket = connect(sockfd, (struct sockaddr *) &server, sizeof(server));
    if(connect_socket < 0) {
        perror("client@eeis:~$ make_tcp_socket:: connect");
        exit(EXIT_FAILURE);
    } else {
        printf("client@eeis:~$ Socket connected! Socket has been connected.\n");
    }

    // advice to input
//    printf("client@eeis:~$ ");

    // create a new program
    pid_t pid = fork();   // return value is child process with zero
    if(pid < 0) {
        perror("client@eeis:~$ create_new_program:: fork");
    }

    // pid == 0, receive data from server; otherwise, send data to server
    if(pid != 0) {
        signal(SIGUSR1, quit);
        while(fgets(send_buf, MAXLINE, stdin) != NULL) {
            send(sockfd, send_buf, strlen(send_buf), 0);   // send data to fd
            memset(send_buf, 0, strlen(send_buf));         // clear the buffer
        }
        close(sockfd);
    } else {
        do {
            int recv_len = read(sockfd, recv_buf, MAXLINE);
            if (recv_len < 0) {
                perror("client@eeis:~$ Wrong Read.");
                break;
            } else if (recv_len == 0) {
                printf("client@eeis:~$ Client closed!\n");
                break;
            }
            recv_buf[recv_len] = '\0';
//            printf("Client recv_buf is %s", recv_buf);
//            printf("strcmp(recv_buf, END) = ? %d.", strcmp(recv_buf, "END"));

            if(strcmp(recv_buf, "END") != 10) {
                if (fputs(recv_buf, stdout) == EOF) {
                    perror("client@eeis:~$ fputs error\n");
                }
            } else {
                printf("client@eeis:~$ Receive END!\n");
                break;
            }
        }while (TRUE);  // when receive "END", stop
        close(sockfd);
        kill(getppid(), SIGUSR1);  // kill progress
    }

    printf("server@eeis:~$ Socket connection teardowns.\n");

    return 0;
}

