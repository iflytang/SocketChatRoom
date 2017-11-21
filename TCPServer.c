#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define MAXLINE 1024
#define TRUE    1

int main(int argc, char **argv)
{
    int sockfd, fd, n, m;
    char line[MAXLINE + 1];    // char array, size is 1024+1
    struct sockaddr_in6 servaddr, cliaddr;   // struct sockaddr_in6, using IPv6
    time_t t0 = time(NULL);
    printf("time #: %ld\n", t0);  // print time as long number
    fputs(ctime(&t0), stdout);  // print time as human readable format

    /*create the socket, AF_ means Address Format, SOCK_STREAM transfers data as pipe*/
    // socket(int namespace, int style, int protocol), return socket file descriptor
    if((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
        perror("socket error");

    /*bind a name to socket*/
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;    // format of socket address
    servaddr.sin6_port = htons(20000);  // port of socket, port == 20000
    servaddr.sin6_addr = in6addr_any;   // address of requested host

    if(bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)    // bind an address to an socket
        perror("bind error");  
  
    if(listen(sockfd, 5) == -1)    // making it as sever socket, argument n specifies
                                   // the length of the queue for pending connections
        perror("listen error");  

    while(TRUE) {

        printf("> Waiting clients ...\r\n");

        socklen_t clilen = sizeof(struct sockaddr);  
        fd = accept(sockfd, (struct sockaddr*)&cliaddr, &clilen);  // accept client request, waiting if there no connection pending
        if(fd == -1)  {  
            perror("accept error");  
        }   

        printf("> Accepted.\r\n");

        while((n = read(fd, line, MAXLINE)) > 0) {
            line[n] = 0;
            if(fputs(line, stdout) == EOF)
                perror("fputs error");
        }
        close(fd);    // close a file
    }
    
    if(n < 0) perror("read error");
       
    exit(0);
}
