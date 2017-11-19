/**
* Created by tsf on 17-11-19.
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
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>

#define MAXLINE 1024
#define TRUE 1
#define SOCKET_PORT 2017
#define PENDING_QUEUE 10

// when interrupted, call this method
void quit(int signal) {
    printf("Socket teardowns!\n");
    exit(EXIT_SUCCESS);
}

// get current time
char * get_time() {
    time_t current_time;
    current_time = time(&current_time);
    char * now = ctime(&current_time);

    return now;
}


int main(int argc, char **argv) {
    int sockfd;
    char recv_buf[MAXLINE + 1] = {0};
    char send_buf[MAXLINE + 1] = {0};
    char * client_addr = "::1";
    char * client_name = "rick";
    struct sockaddr_in6 server;

    // read the server_addr and server_name from **agrv
    if (argc != 3) {
        perror("wrong input!\nusage:\n\t./x.o <IPv6 Address> <String name>\n");
        exit(EXIT_FAILURE);
    } else {
        client_addr = argv[1];
        client_name = argv[2];
    }

    // print log_in information
    char * current_time = get_time();     // get the current system time
    printf("%s login from %s at %s", client_name, client_addr, current_time);

    // create a socket
    sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("make_tcp_connection:: socket");
        exit(EXIT_FAILURE);
    }

    // give the socket a name
    bzero(&server, sizeof(server));
    server.sin6_family = AF_INET6;
    server.sin6_port = htons(SOCKET_PORT);
    inet_pton(AF_INET6, client_addr, &server.sin6_addr);

    // connect the server socket
    int connect_socket = connect(sockfd, (struct sockaddr *) &server, sizeof(server));
    if(connect_socket < 0) {
        perror("make_tcp_socket:: connect");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();    // create a new progress, pid > 0, SEND_DATA; pid = 0, RECEIVE_DATA

    char usr_info[MAXLINE + 100] = {0};
    while (TRUE) {
        if (pid > 0) {
            // SEND_DATA
            // interrupt to quit
            signal(SIGUSR1, quit);
            // clear buf
            bzero(usr_info, strlen(usr_info));
            bzero(send_buf, strlen(send_buf));
            // store usr_info, and send
            strcpy(usr_info, argv[2]);    // read the usr_name
            strcat(usr_info, ": ");
            fgets(send_buf, MAXLINE, stdin);
            strcat(usr_info, send_buf);
            if (send(sockfd, usr_info, strlen(usr_info), 0) < 0) {
                perror("fail to send from client.");
                exit(EXIT_FAILURE);
            }
        } else {
            // RECEIVE_DATA
            int recv_len = read(sockfd, recv_buf, MAXLINE);
            if (recv_len <= 0) {
                perror("wrong read from client.");
                close(sockfd);
                kill(getppid(), SIGUSR1);   // kill parent progress, permit send data
                exit(EXIT_FAILURE);

            }

            // check "EDN" with strstr(), if return NULL pointer, then not include "END"
            if(strstr(recv_buf, "END") == NULL) {
                fputs(recv_buf, stdout);
                bzero(recv_buf, strlen(recv_buf));
            } else {
                printf("Receive END!\n");
                close(sockfd);
                kill(getppid(), SIGUSR1);   // kill parent progress, permit send data
                exit(EXIT_FAILURE);
                }
            }
        }

    return 0;
}
