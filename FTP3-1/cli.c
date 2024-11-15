#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>

#define MAX_BUF 20
#define CONT_PORT 20001

// //////////////////////////////////////////////////////////////////////////////////////
// File Name : cli.c
// Date : 2024/05/20
// OS : Ubuntu 20.04.6 64-bit
//
// Arthor : Min Seok Choi
// Student ID : 2020202090
//
// Title: System Programming Assignment 3-1 (FTP server)
// Description: The purpose of Assignment 3-1 is to implement log-in sequence (client)
// //////////////////////////////////////////////////////////////////////////////////////

void log_in(int sockfd);

int main(int argc, char *argv[])
{
    int sockfd, n, p_pid;
    struct sockaddr_in servaddr;
    char *haddr = argv[1];
    int portno = atoi(argv[2]);

    // create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("failed to create socket\n");
        return 0;
    }

    // set server address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(portno);

    // convert ip address
    if (inet_pton(AF_INET, haddr, &servaddr.sin_addr) <= 0)
    {
        printf("failed to convert ip\n");
        close(sockfd);
        return 0;
    }

    // connect to server
    if (connect(sockfd, (struct sockaddr *)&servaddr, (int)sizeof(servaddr)) < 0)
    {
        printf("failed to connect to server\n");
        close(sockfd);
        return 0;
    }

    log_in(sockfd);
    close(sockfd);

    return 0;
}

void log_in(int sockfd)
{
    int n;
    char user[MAX_BUF];
    char passwd[MAX_BUF];
    char *temp;
    char buf[MAX_BUF];

    // try to login
    for (int i = 0; i < 3; i++)
    {
        // get username
        temp = getpass("Input user : ");
        strcpy(user, temp);
        // printf("%s\n", user);

        // get passwd
        temp = getpass("Input passwd : ");
        strcpy(passwd, temp);
        // printf("%s\n", passwd);

        // write to server
        write(sockfd, user, MAX_BUF);
        write(sockfd, passwd, MAX_BUF);

        // read login result from server
        n = read(sockfd, buf, MAX_BUF);
        buf[n] = '\0';

        if (strcmp(buf, "OK") == 0) // login success
        {
            printf("** Success to log-in **\n");
            return;
        }
        else if (strcmp(buf, "FAIL") == 0) // login failed
        {
            printf("** Log-in failed **\n");
        }
        else if (strcmp(buf, "DISCONNECTION") == 0) // login failed 3 times
        {
            printf("** Connection closed **\n");
            return;
        }
    }
}
