/**
* Created by tsf on 17-11-18.
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
#define PERM S_IRUSR | S_IWUSR

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

// create shared memory
int create_shared_memory() {
    int shared_memory_id = shmget(IPC_PRIVATE, MAXLINE + 1, PERM);
    if(shared_memory_id < 0) {
        perror("fail to create shared memory.\n");
        exit(EXIT_FAILURE);
    }
    return shared_memory_id;
}


int main(int argc, char **argv) {
    // define parameter of server socket
    int sockfd_server, sockfd_client;  // server socket file descriptor
    char recv_buf[MAXLINE + 1] = {0};
    char send_buf[MAXLINE + 1] = {0};
    char WELCOME[MAXLINE + 1] = "|================ Chating room ===============|";
    char * server_addr = "::1";       // server's default ipv6 address
    char * server_name = "Alice";    // server's name when in communication
    struct sockaddr_in6 server, client;     // attributes of server and client

    // create shared memory
    int shmid;
    shmid = create_shared_memory();

    // read the server_addr and server_name from **agrv
    if (argc != 3) {
        perror("wrong input!\nusage:\n\t./x.o <IPv6 Address> <String name>\n");
        exit(EXIT_FAILURE);
    } else {
        server_addr = argv[1];
        server_name = argv[2];
    }

    // print log_in information
    char * current_time = get_time();     // get the current system time
    printf("monitor %s login from %s at %s", server_name, server_addr, current_time);

    // create and check a server socket
    sockfd_server = socket(AF_INET6, SOCK_STREAM, 0);
    if(sockfd_server < 0) {
        perror("make_tcp_connection:: socket");
        exit(EXIT_FAILURE);
    }

    // give the socket a name
    bzero(&server, sizeof(server));
    server.sin6_family = AF_INET6;
    server.sin6_port = htons(SOCKET_PORT);
    inet_pton(AF_INET6, server_addr, &server.sin6_addr);

    // bind socket
    int bind_socket = bind(sockfd_server, (struct sockaddr *) &server, sizeof(server));
    if(bind_socket < 0) {
        perror("make_tcp_connection:: bind");
        exit(EXIT_FAILURE);
    }

    // listen socket
    int listen_socket = listen(sockfd_server, PENDING_QUEUE);  // making it as server socket
    if(listen_socket < 0) {
        perror("make_tcp_connection:: listen");
        exit(EXIT_FAILURE);
    }
    printf("listening...\n|================ Chating room monitor ===============|\n");

    // accept socket in while, and process
    int online_people = 0;
    char str[12];  // store online_people in char format
    char * write_addr;
    char * read_addr;
    char * temp;
    while (TRUE) {
        // accept a client socket
        socklen_t client_len = sizeof(struct sockaddr);
        sockfd_client = accept(sockfd_server, (struct sockaddr *) &client, &client_len);
        if(sockfd_client < 0) {
            perror("make_tcp_connection:: accept");
            exit(EXIT_FAILURE);
        }

        // send welcome and online people
        char online[255] = "online people: ";
        sprintf(str, "%d", ++online_people);   // convert online_people into chars
        strcat(online, str);
        strcat(WELCOME, online);
        send(sockfd_client, WELCOME, strlen(WELCOME) ,0);     // send welcome with online people
        printf("test ==> %s\n", WELCOME);
        printf("online numbers: %d\n", online_people);
        // clear WELCOME and online
        bzero(WELCOME, strlen(WELCOME));
        strcpy(WELCOME, "|================ Chating room ===============|");
        bzero(online, strlen(online));
        strcpy(online, "online people: ");
        bzero(str, strlen(str));


        /** create a new progress, parent progress handle SEND_WRITE_DATA,
         *  child progress handle RECEIVE_WRITE_SEND_DATA with pid = 0
         */
        pid_t pid = fork();  // return child progress with pid = 0
//        if(pid > 0) {
            // SEND, then WRITE_DATA_TO_SHARED_MEMORY
//            signal(SIGUSR1, quit);
//            while (fgets(send_buf, MAXLINE, stdin) != NULL) {
//                write_addr = shmat(shmid, 0, 0);
//                bzero(write_addr, MAXLINE);   // clear the shared memory
//
//                strncpy(write_addr, send_buf, MAXLINE);
//                bzero(send_buf, MAXLINE);     // clear the send buffer
//            }
//        }

        if(pid == 0){
            pid_t ppid = fork(); // return child process of child process with ppid = 0
            while (TRUE) {
                if (ppid > 0) {
                    // RECEIVE_DATA, then WRITE_DATA_TO_SHARED_MEMORY
                    // receive data
                    bzero(recv_buf, MAXLINE);
                    int recv_len = read(sockfd_client, recv_buf, MAXLINE);
                    if(recv_len <= 0) {
                        perror("wrong receive from socket.\n");
                        close(sockfd_client);
                        kill(ppid, SIGUSR1);  // kill its child process, permit read data from SM to send
                        kill(getppid(), SIGUSR1); // kill its father process, permit send data to SM to write
                        exit(EXIT_FAILURE);
                    }

                    // end session if receive "END" substring
                    if(strstr(recv_buf, "END") != NULL) {
                        perror("Receive End.\n");
                        close(sockfd_client);
                        kill(ppid, SIGUSR1);  // kill its child process, permit read data from SM to send
                        kill(getppid(), SIGUSR1); // kill its father process, permit send data to SM to write
                        exit(EXIT_FAILURE);
                    }

                    // store data in SM
                    write_addr = shmat(shmid, 0, 0);
                    bzero(write_addr, MAXLINE);
                    strncpy(write_addr, recv_buf, MAXLINE);

                    // display
                    char * now = get_time();
                    printf("%s %s\n", now, recv_buf);

                } else if(ppid == 0){
                    //SEND_DATA_TO_CLIENTS
                    signal(SIGUSR1, quit);
                    sleep(1);
                    read_addr = shmat(shmid, 0, 0);

                    if(strcmp(temp, read_addr) != 0) {
                        strcpy(temp, read_addr);

                        char now[255 + MAXLINE] = {0};
                        strcpy(now, get_time());
                        strcat(now, read_addr);

                        send(sockfd_client, now, strlen(now), 0);
                        printf("send from server: %s\n", now);

                        bzero(read_addr, MAXLINE);
                        strcpy(read_addr, temp);
                    }
                }
            }
        }

    }

    close(sockfd_server);
    close(sockfd_client);

    return 0;
}