/**
* Created by tsf on 17-11-18.
*/

#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
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
#define TRUE 1

// when interrupted, call this method
void quit(int signal) {
    printf("> Socket teardowns!\n");
    exit(EXIT_SUCCESS);
}

// get current time
char * get_cur_time() {
    time_t current_time;
    current_time = time(&current_time);
    char * now = ctime(&current_time);

    return now;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in6 client;
    int clientfd;
    char * client_addr = "::1";
    char * client_name = "rick";
    char buf_recv[MAXLINE] = {0};
    char buf_read[MAXLINE] = {0};

    // read the server_addr and server_name from **agrv
    if (argc != 3) {
        perror("> wrong input!\nusage:\n\t./x.o <IPv6 Address> <String name>\n");
        exit(EXIT_FAILURE);
    } else {
        client_addr = argv[1];
        client_name = argv[2];
    }

    // create a socket
    clientfd = socket(AF_INET6, SOCK_STREAM, 0);
    if(clientfd < 0) {
        perror("> make_tcp_connection:: socket");
        exit(EXIT_FAILURE);
    }

    // give the socket a name
    bzero(&client, sizeof(client));
    client.sin6_family = AF_INET6;
    client.sin6_port = htons(SOCKET_PORT);
    inet_pton(AF_INET6, client_addr, &client.sin6_addr);

    // connect the server socket
    int connect_socket = connect(clientfd, (struct sockaddr *) &client, sizeof(client));
    if(connect_socket < 0) {
        perror("> make_tcp_socket:: connect");
        exit(EXIT_FAILURE);
    }


    // read WELCOME from server
    int recv_len = read(clientfd, buf_recv, MAXLINE);
    if (recv_len <= 0) {
        perror("> wrong read from client.");
        exit(EXIT_FAILURE);
    }
    printf("%s\n", buf_recv);

    // print log_in information
    char * current_time = get_cur_time();     // get the current system time
    printf("> user %s login from %s at %s", client_name, client_addr, current_time);

    pid_t pid = fork();
    // pid > 0, SEND DATA; pid = 0, RECEIVE DATA
    while (TRUE) {
        if (pid > 0) {
            signal(SIGUSR1, quit);
            // clear
            bzero(buf_read, MAXLINE);
            // add user's info
            strcpy(buf_recv, client_name);
            strcat(buf_recv, ": ");
            // read data from console, then send to socket
            if(fgets(buf_read, MAXLINE, stdin) != NULL) {
                strncat(buf_recv, buf_read, strlen(buf_read) - 1);
                send(clientfd, buf_recv, strlen(buf_recv), 0);
            } else {
                perror("> wrong read from console.\n");
                exit(EXIT_FAILURE);
            }
        } else if (pid == 0) {
            // clear
            bzero(buf_recv, MAXLINE);
            // check recv_len
            int recv_len = read(clientfd, buf_recv, MAXLINE);
            if (recv_len <= 0) {
                perror("> socket closed by remote peer.\n");
                kill(getppid(), SIGUSR1);    // kill parent progress
                break;
            }
            // check "END"
            if(strstr(buf_recv, "END") != NULL) {
                perror("> socket closed by remote peer.\n");
                kill(getppid(), SIGUSR1);    // kill parent progress
                break;
            }
            printf("%s\n", buf_recv);
        }
    }

    close(clientfd);

    return 0;
}