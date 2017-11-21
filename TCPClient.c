#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define MAXLINE 1024
#define TRUE    1

int main(int argc, char **argv)
{
    int sockfd, n, m;
    char line[MAXLINE + 1];
    struct sockaddr_in6 servaddr;
    time_t t0 = time(NULL);
    printf("time #: %ld\n", t0);
    fputs(ctime(&t0), stdout);
    
    if(argc != 2)
        perror("usage: a.out <IPaddress>");
    
    if((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
        perror("socket error");
       
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_port = htons(20000);

    // argv[1] == IPV6 Address (client address)
    if(inet_pton (AF_INET6, argv[1], &servaddr.sin6_addr) <= 0) 
        perror("inet_pton error");
       
    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        perror("connect error");
       
    while(fgets(line, MAXLINE, stdin) != NULL) {
        send(sockfd, line, strlen(line), 0);
    }
    
    close(sockfd);

    exit(0);
}
