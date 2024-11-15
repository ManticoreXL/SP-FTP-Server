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

#define BUFSIZE 1024
#define RCVSIZE 4096

// //////////////////////////////////////////////////////////////////////////////////////
// File Name : cli.c
// Date : 2024/05/26
// OS : Ubuntu 20.04.6 64-bit
//
// Arthor : Min Seok Choi
// Student ID : 2020202090
//
// Title: System Programming Assignment 3-2 (FTP server)
// Description: The purpose of Assignment 3-2 is to implement FTP system
//              with separate control connection and data connection (client)
// //////////////////////////////////////////////////////////////////////////////////////

int conv_cmd(char *buff, char *cmd_buff);
char *convert_addr_to_str(unsigned long ip_addr, unsigned int *port);

int main(int argc, char **argv)
{
    int ctrfd, datfd, tempfd;
    int datport, reply;
    int bcnt = 0;
    char *haddr = argv[1];
    char *hostport;
    socklen_t ctrlen, datlen, templen;
    struct sockaddr_in ctraddr, dataddr, tempaddr;
    char buf[BUFSIZE];
    char cmd[BUFSIZE];
    char ack[BUFSIZE];
    char rcv[RCVSIZE];

    if (argc < 3)
    {
        printf("server address and port number are required\n");
        return 0;
    }

    // create control conncetion socket
    if ((ctrfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("failed to create control connection socket\n");
        return 0;
    }

    // set control connection address
    ctrlen = sizeof(ctraddr);
    memset(&ctraddr, 0, ctrlen);
    ctraddr.sin_family = AF_INET;
    inet_pton(AF_INET, haddr, &ctraddr.sin_addr);
    ctraddr.sin_port = htons(atoi(argv[2]));

    // make control connection
    ctrlen = sizeof(ctraddr);
    if (connect(ctrfd, (struct sockaddr *)&ctraddr, ctrlen) < 0)
    {
        printf("failed to connect to server\n");
        close(ctrfd);
        return 0;
    }

    // create data connection temporary
    write(1, "> ", 2);
    while (1)
    {
        // flush buffers
        bzero(buf, sizeof(buf));
        bzero(cmd, sizeof(cmd));
        bzero(ack, sizeof(ack));
        bzero(rcv, sizeof(rcv));

        // generate random port number
        hostport = convert_addr_to_str(ctraddr.sin_addr.s_addr, &datport);

        // set data connection address
        datlen = sizeof(dataddr);
        memset(&dataddr, 0, datlen);
        dataddr.sin_family = AF_INET;
        inet_pton(AF_INET, haddr, &dataddr.sin_addr);
        dataddr.sin_port = htons(datport);

        // create data connection socket and bind
        datfd = socket(AF_INET, SOCK_STREAM, 0);
        if (bind(datfd, (struct sockaddr *)&dataddr, datlen) < 0)
        {
            printf("Failed to bind local address\n");
            break;
        }

        // read command from user
        read(0, buf, BUFSIZE);
        conv_cmd(buf, cmd);

        // make data connection
        listen(datfd, 5);

        // check quick quit sequence
        if (strcmp(cmd, "QUIT") == 0)
        {
            write(ctrfd, cmd, strlen(hostport));
            read(ctrfd, ack, RCVSIZE);
            printf("%s", ack);
            close(ctrfd);
            close(datfd);
            close(tempfd);
            exit(0);
        }
        else
        {
            // send hostport and receive ACK
            write(ctrfd, hostport, strlen(hostport));
            printf("converting to %s\n", hostport);
        }

        // accept from server
        templen = sizeof(tempaddr);
        tempfd = accept(datfd, (struct sockaddr *)&tempaddr, &templen);
        if (tempfd < 0)
        {
            printf("failed to accept data connection\n");
            break;
        }

        bcnt += read(ctrfd, ack, BUFSIZE);
        printf("%s", ack); // 200
        bzero(ack, sizeof(ack));

        // send ftp command and receive ACK
        write(ctrfd, cmd, strlen(cmd));
        bcnt += read(ctrfd, ack, BUFSIZE);
        printf("%s", ack); // 150
        bzero(ack, sizeof(ack));

        // receive ftp result from server and receive ACK
        bcnt += read(tempfd, rcv, RCVSIZE);
        printf("%s", rcv);
        bcnt += read(ctrfd, ack, BUFSIZE);
        printf("%s", ack); // 226
        bzero(ack, sizeof(ack));

        // print byte count
        printf("OK. %d bytes is received.\n", bcnt);

        close(datfd);
        close(tempfd);

        write(1, "> ", 2);
    }

    // close socket descriptors
    if (ctrfd >= 0)
        close(ctrfd);
    if (datfd >= 0)
        close(datfd);
    if (tempfd >= 0)
        close(tempfd);

    // free hostport
    if (hostport != NULL)
        free(hostport);

    return 0;
}

char *convert_addr_to_str(unsigned long ip_addr, unsigned int *port)
{
    char *ip_str, *addr;
    struct in_addr ip_struct;
    ip_struct.s_addr = ip_addr;
    int ip_bytes[4];
    int random_port = 0;
    unsigned porthi = 0;
    unsigned portlo = 0;

    // convert ip address to string
    ip_str = inet_ntoa(ip_struct);
    if (ip_str == NULL)
        return NULL;

    // generate random port number 10001~30000
    srand(time(NULL));
    random_port = (rand() & 20000) + 10001;
    *port = random_port;

    // allocate memory for result
    addr = (char *)malloc(50);
    if (addr == NULL)
        return NULL;

    // concatenate ip and port
    sscanf(ip_str, "%u.%u.%u.%u", &ip_bytes[0], &ip_bytes[1], &ip_bytes[2], &ip_bytes[3]);

    // split random port to two parts
    porthi = random_port / 256;
    portlo = random_port % 256;

    // format the result string
    snprintf(addr, 50, "%u, %u, %u, %u, %u, %u", ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3], porthi, portlo);

    // return result addr string
    return addr;
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