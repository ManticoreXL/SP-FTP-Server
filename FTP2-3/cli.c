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

// //////////////////////////////////////////////////////////////////////////////////////
// File Name : cli.c
// Date : 2024/05/13
// OS : Ubuntu 20.04.6 64-bit
//
// Arthor : Min Seok Choi
// Student ID : 2020202090
//
// Title: System Programming Assignment 2-3 (FTP server)
// Description: The purpose of Assignment 2-3 is to implement socket communication
//              ftp file system with fork(). (client)
// //////////////////////////////////////////////////////////////////////////////////////

#define BUFSIZE 1024
#define RCVSIZE 4096

char buff[BUFSIZE];
char cmd_buff[BUFSIZE];
char rcv_buff[BUFSIZE];
int server_fd, client_fd;

int conv_cmd(char *buff, char *cmd_buff);
void sh_sigint(int signum);

int main(int argc, char **argv)
{
    int len, len_cmd, len_rcv;
    struct sockaddr_in server_addr;
    char *haddr;
    int portno = 0;

    if (argc < 3)
    {
        write(2, "Server address and port number are required\n", strlen("Server address and port number are required.\n"));
        exit(1);
    }

    // get server address and portno from argv
    haddr = argv[1];
    portno = atoi(argv[2]);

    // init buffers to zero
    bzero(buff, strlen(cmd_buff));
    bzero(cmd_buff, strlen(cmd_buff));
    bzero(rcv_buff, strlen(rcv_buff));

    // set socket descriptor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // set server address
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(haddr);
    server_addr.sin_port = htons(portno);

    // connect to server
    if (connect(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        write(2, "Can't connect\n", strlen("Can't connect\n"));
        exit(1);
    }

    // read command from user
    write(1, "> ", 2);
    while ((len = read(0, buff, sizeof(buff))) > 0)
    {
        // convert user command into ftp command
        if (conv_cmd(buff, cmd_buff) < 0)
        {
            write(2, "conv_cmd() error!!\n", strlen("conv_cmd() error!!\n"));
            exit(1);
        }

        // write the cmd to server
        len_cmd = strlen(cmd_buff);
        if (write(server_fd, cmd_buff, len_cmd) != len_cmd)
        {
            write(2, "write() error!!\n", strlen("write() error!!\n"));
            exit(1);
        }

        // read the result from server
        if ((len_rcv = read(server_fd, rcv_buff, RCVSIZE)) < 0)
        {
            write(2, "read() error!!\n", strlen("read() error!!\n"));
            exit(1);
        }

        // display process result
        write(1, rcv_buff, strlen(rcv_buff));
        write(1, "\n", 1);
        write(1, "> ", 2);

        // clear buffer
        memset(buff, 0, strlen(buff));
        memset(cmd_buff, 0, strlen(cmd_buff));
        memset(rcv_buff, 0, strlen(rcv_buff));
    }
    close(server_fd);
    return 0;
}

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
        // NLST
        strcat(cmd_buff, "NLST");

        // concatenation arguments (options, path)
        char *arg = strtok(NULL, " ");
        while (arg != NULL)
        {
            strcat(cmd_buff, " ");
            strcat(cmd_buff, arg);
            arg = strtok(NULL, " ");
        }

        return 0;
    }
    else if (strcmp(cmd, "pwd") == 0)
    {
        // PWD
        strcat(cmd_buff, "PWD");
        return 0;
    }
    else if (strcmp(cmd, "dir") == 0)
    {
        // LIST
        strcat(cmd_buff, "LIST");
        return 0;
    }
    else if (strcmp(cmd, "cd") == 0)
    {
        // CDUP
        char *arg = strtok(NULL, " ");
        if (strcmp(arg, "..") == 0)
        {
            strcat(cmd_buff, "CDUP");
            return 0;
        }

        // CWD
        strcat(cmd_buff, "CWD");

        // concatenation arguments
        arg = strtok(NULL, " ");
        while (arg != NULL)
        {
            strcat(cmd_buff, " ");
            strcat(cmd_buff, arg);
            arg = strtok(NULL, " ");
        }
    }
    else if (strcmp(cmd, "mkdir") == 0)
    {
        // MKD
        strcat(cmd_buff, "MKD");

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
    else if (strcmp(cmd, "delete") == 0)
    {
        // DELE
        strcat(cmd_buff, "DELE");

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
    else if (strcmp(cmd, "rmdir") == 0)
    {
        // RMD
        strcat(cmd_buff, "RMD");

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
    else if (strcmp(cmd, "rename") == 0)
    {
        // RNFR + arg1
        strcat(cmd_buff, "RNFR");
        char *arg = strtok(NULL, " ");
        strcat(cmd_buff, arg);

        // RNTO + arg2
        arg = strtok(NULL, " ");
        strcat(cmd_buff, arg);

        return 0;
    }
    else if (strcmp(cmd, "quit") == 0)
    {
        // QUIT
        strcat(cmd_buff, "QUIT");
        return 0;
    }

    return 0;
}

// signal handler for SIGINT
void sh_sigint(int signum)
{
    printf("SIGINT received. send QUIT to server...\n");
    memset(buff, 0, strlen(buff));
    strcpy(buff, "QUIT");
    write(server_fd, buff, strlen(buff));
    exit(0);
}