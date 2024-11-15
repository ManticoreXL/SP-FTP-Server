#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <limits.h>

#define BUFFSIZE 1024
#define RCV_BUFFSIZE 4096

// //////////////////////////////////////////////////////////////////////////////////////
// File Name : cli.c
// Date : 2024/05/01
// OS : Ubuntu 20.04.6 64-bit
//
// Arthor : Min Seok Choi
// Student ID : 2020202090
//
// Title: System Programming Assignment 2-1 (FTP server)
// Description: The purpose of Assignment 2-1 is to implement socket communication
//              ftp file system (client)
// //////////////////////////////////////////////////////////////////////////////////////

// convert command to ftp command
int conv_cmd(char *buff, char *cmd_buff)
{
    if (buff == NULL || cmd_buff == NULL)
        return 0;

    char *line = strtok(buff, "\n");
    char *cmd = strtok(line, " ");

    // convert command to ftp command
    if (strcmp(cmd, "ls") == 0)
    {
        strcat(cmd_buff, "NLST");

        // concatenation arguments
        char *arg = strtok(NULL, " ");
        while (arg != NULL)
        {
            strcat(cmd_buff, " ");
            strcat(cmd_buff, arg);
            arg = strtok(NULL, " ");
        }

        return 0;
    }
    else if (strcmp(cmd, "quit") == 0)
    {
        strcat(cmd_buff, "QUIT");
        return 0;
    }
}

int main(int argc, char **argv)
{
    int socket_fd, client_fd;
    int len, len_cmd, len_out;
    struct sockaddr_in server_addr;
    char *haddr;
    int portno = 0;
    char buff[BUFFSIZE];
    char cmd_buff[BUFFSIZE];
    char rcv_buff[RCV_BUFFSIZE];

    if (argc < 3)
    {
        write(2, "Server address and port number are required\n", strlen("Server address and port number are required.\n"));
        exit(1);
    }

    // get server address and portno from argv
    haddr = argv[1];
    portno = atoi(argv[2]);

    if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        write(2, "Can't open stream socket\n", strlen("Can't open stream socket\n"));
        exit(1);
    }

    // init buffers to zero
    bzero(buff, sizeof(cmd_buff));
    bzero(cmd_buff, sizeof(cmd_buff));
    bzero(rcv_buff, sizeof(rcv_buff));

    // set server address
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(haddr);
    server_addr.sin_port = htons(portno);

    // connect to server
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        write(2, "Can't connect\n", strlen("Can't connect\n"));
        exit(1);
    }

    write(1, "> ", 2);
    while ((len = read(0, buff, sizeof(buff))) > 0)
    {
        if (conv_cmd(buff, cmd_buff) < 0)
        {
            write(2, "conv_cmd() error!!\n", strlen("conv_cmd() error!!\n"));
            exit(1);
        }

        // write the cmd to server
        len_cmd = strlen(cmd_buff);
        if (write(socket_fd, cmd_buff, len_cmd) != len_cmd)
        {
            write(2, "write() error!!\n", strlen("write() error!!\n"));
            exit(1);
        }

        // read the result from server
        if ((len_out = read(socket_fd, rcv_buff, BUFFSIZE)) < 0)
        {
            write(2, "read() error!!\n", strlen("read() error!!\n"));
            exit(1);
        }

        // display process result
        write(1, rcv_buff, strlen(rcv_buff));
        write(1, "\n", 1);
        write(1, "> ", 2);

        // clear buffer
        bzero(buff, sizeof(cmd_buff));
        bzero(cmd_buff, sizeof(cmd_buff));
        bzero(rcv_buff, sizeof(rcv_buff));
    }

    close(socket_fd);
    return 0;
}