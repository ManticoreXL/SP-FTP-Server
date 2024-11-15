#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <signal.h>

#define BUF_SIZE 256

// //////////////////////////////////////////////////////////////////////////////////////
// File Name : cli.c
// Date : 2024/05/06
// OS : Ubuntu 20.04.6 64-bit
//
// Arthor : Min Seok Choi
// Student ID : 2020202090
//
// Title: System Programming Assignment 2-2 (FTP server)
// Description: The purpose of Assignment 2-2 is to implement socket communication
//              ftp file system with fork() (client)
// //////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
    char buff[BUF_SIZE];
    int n, sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    // connect to server
    connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    while (1)
    {
        write(1, "> ", 2);
        bzero(buff, BUF_SIZE);
        read(0, buff, BUF_SIZE);

        if (write(sockfd, buff, BUF_SIZE) > 0)
        {
            if (read(sockfd, buff, BUF_SIZE) > 0)
                printf("from server:%s", buff);
            else
                break;
        }
        else
        {
            break;
        }

        // flush buffer
        bzero(buff, sizeof(buff));
    }
    close(sockfd);
    return 0;
}