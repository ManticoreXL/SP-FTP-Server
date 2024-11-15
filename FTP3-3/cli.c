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
// Date : 2024/06/04
// OS : Ubuntu 20.04.6 64-bit
//
// Arthor : Min Seok Choi
// Student ID : 2020202090
//
// Title: System Programming Assignment 3-3 (FTP server)
// Description: The purpose of Assignment 3-3 is to implement FTP system completely (client)
// //////////////////////////////////////////////////////////////////////////////////////

int conv_cmd(char *buff, char *cmd_buff);
int log_in(int srvfd);
char *convert_addr_to_str(unsigned long ip_addr, unsigned int *port);

int main(int argc, char **argv)
{
    int ctrfd, datfd, tempfd;
    int datport;
    int reply, bcnt = 0;
    char *haddr = argv[1];
    char *hostport;
    struct sockaddr_in ctraddr, dataddr, tempaddr;
    socklen_t ctrlen, datlen, templen;
    char buf[BUFSIZE];
    char cmd[BUFSIZE];
    char rcv[RCVSIZE];
    char rsp[BUFSIZE];
    char *comm;
    bzero(buf, sizeof(buf));
    bzero(cmd, sizeof(cmd));
    bzero(rcv, sizeof(rcv));
    bzero(rsp, sizeof(rsp));

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
    write(1, "Connected to sswlab.kw.ac.kr.\n", strlen("Connected to sswlab.kw.ac.kr.\n"));

    // read welcome message from server
    read(ctrfd, rsp, BUFSIZE);
    printf("%s", rsp);
    bzero(rsp, sizeof(rsp));

    // log-in sequence
    if (log_in(ctrfd) < 0)
        return 0;

    while (1)
    {
        write(1, "> ", 2);
        // flush buffers
        bzero(buf, sizeof(buf));
        bzero(cmd, sizeof(cmd));
        bzero(rcv, sizeof(rcv));
        bzero(rsp, sizeof(rsp));
        bcnt = 0;
        comm = NULL;

        // generate random port number
        hostport = convert_addr_to_str(ctraddr.sin_addr.s_addr, &datport);

        // set data connection address
        datlen = sizeof(dataddr);
        memset(&dataddr, 0, datlen);
        dataddr.sin_family = AF_INET;
        inet_pton(AF_INET, haddr, &dataddr.sin_addr);
        dataddr.sin_port = htons(datport);

        // read command from user
        read(0, buf, BUFSIZE);
        if (conv_cmd(buf, cmd) < 0)
        {
            printf("failed to conv_cmd()\n");
            break;
        }
        write(ctrfd, cmd, strlen(cmd));
        comm = strtok(cmd, " ");
        bzero(buf, sizeof(buf));

        // cmd-sensitive read/write
        if (strcmp(comm, "NLST") == 0)
        {
            // create data connection socket and bind
            if ((datfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                printf("failed to create socket.\n");
                break;
            }
            if (bind(datfd, (struct sockaddr *)&dataddr, datlen) < 0)
            {
                printf("failed to bind local address.\n");
                break;
            }

            // open data connection
            write(ctrfd, hostport, strlen(hostport));
            listen(datfd, 5);
            templen = sizeof(tempaddr);
            tempfd = accept(datfd, (struct sockaddr *)&tempaddr, &templen);

            // receive reply // 200
            bcnt += read(ctrfd, rsp, BUFSIZE);
            write(1, rsp, strlen(rsp));
            if (strncmp(rsp, "550", 3) == 0)
                continue;
            bzero(rsp, sizeof(rsp));
            write(ctrfd, "OK\n", strlen("OK\n"));

            // receive reply // 150
            bcnt += read(ctrfd, rsp, BUFSIZE);
            write(1, rsp, strlen(rsp));
            bzero(rsp, sizeof(rsp));
            write(ctrfd, "OK\n", strlen("OK\n"));

            // receive ftp result
            bcnt += read(tempfd, rcv, RCVSIZE);
            write(1, rcv, strlen(rcv));
            bzero(rcv, sizeof(rcv));
            write(ctrfd, "OK\n", strlen("OK\n"));

            // receive reply // 226
            bcnt += read(ctrfd, rsp, BUFSIZE);
            write(1, rsp, strlen(rsp));
            if (strncmp(rsp, "550", 3) == 0)
                continue;
            bzero(rsp, sizeof(rsp));
            write(ctrfd, "OK\n", strlen("OK\n"));

            // print total number of bytes received
            printf("OK. %d bytes is received.\n", bcnt);

            // close data connection
            close(tempfd);
        }
        else if (strcmp(comm, "RETR") == 0)
        {
            // create data connection socket and bind
            if ((datfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                printf("failed to create socket.\n");
                break;
            }
            if (bind(datfd, (struct sockaddr *)&dataddr, datlen) < 0)
            {
                printf("failed to bind local address.\n");
                break;
            }

            // open data connection
            write(ctrfd, hostport, strlen(hostport));
            listen(datfd, 5);
            templen = sizeof(tempaddr);
            tempfd = accept(datfd, (struct sockaddr *)&tempaddr, &templen);

            // receive reply // 200
            bcnt += read(ctrfd, rsp, BUFSIZE);
            write(1, rsp, strlen(rsp));
            if (strncmp(rsp, "550", 3) == 0)
                continue;
            bzero(rsp, sizeof(rsp));
            write(ctrfd, "OK\n", strlen("OK\n"));

            // receive reply // 150
            bcnt += read(ctrfd, rsp, BUFSIZE);
            write(1, rsp, strlen(rsp));
            bzero(rsp, sizeof(rsp));
            write(ctrfd, "OK\n", strlen("OK\n"));

            // receive ftp result
            bcnt += read(tempfd, rcv, RCVSIZE);
            // write(1, rcv, strlen(rcv));
            write(1, "file received from server\n", strlen("file received from server\n"));
            bzero(rcv, sizeof(rcv));
            write(ctrfd, "OK\n", strlen("OK\n"));

            // receive reply // 226
            bcnt += read(ctrfd, rsp, BUFSIZE);
            write(1, rsp, strlen(rsp));
            if (strncmp(rsp, "550", 3) == 0)
                continue;
            bzero(rsp, sizeof(rsp));
            write(ctrfd, "OK\n", strlen("OK\n"));

            // copy file
            char *arg = strtok(NULL, "\n");
            FILE *file = fopen(arg, "w");
            if (file == NULL)
            {
                printf("failed to create file\n");
                continue;
            }
            size_t bwrite = strlen(rcv);
            size_t bwritten = fwrite(rcv, 1, bwrite, file);
            if (bwrite != bwritten)
            {
                printf("failed to write file\n");
                fclose(file);
                continue;
            }

            // print total number of bytes received
            printf("OK. %d bytes is received.\n", bcnt);

            // close data connection
            close(tempfd);
            fclose(file);
        }
        else if (strcmp(comm, "STOR") == 0)
        {
            // create data connection socket and bind
            if ((datfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                printf("failed to create socket.\n");
                break;
            }
            if (bind(datfd, (struct sockaddr *)&dataddr, datlen) < 0)
            {
                printf("failed to bind local address.\n");
                break;
            }

            // open data connection
            write(ctrfd, hostport, strlen(hostport));
            listen(datfd, 5);
            templen = sizeof(tempaddr);
            tempfd = accept(datfd, (struct sockaddr *)&tempaddr, &templen);

            // receive reply // 200
            bcnt += read(ctrfd, rsp, BUFSIZE);
            write(1, rsp, strlen(rsp));
            if (strncmp(rsp, "550", 3) == 0)
                continue;
            bzero(rsp, sizeof(rsp));
            write(ctrfd, "OK\n", strlen("OK\n"));

            // receive reply // 150
            bcnt += read(ctrfd, rsp, BUFSIZE);
            write(1, rsp, strlen(rcv));
            bzero(rsp, sizeof(rsp));
            write(ctrfd, "OK\n", strlen("OK\n"));

            // copy file content
            char *arg = strtok(NULL, " ");
            if (arg == NULL)
                continue;
            bzero(rcv, sizeof(rcv));
            strcat(rcv, arg);
            strcat(rcv, " ");
            FILE *file = fopen(arg, "r");
            char fbuf[RCVSIZE];
            bzero(fbuf, sizeof(fbuf));
            size_t bytes = fread(fbuf, 1, RCVSIZE - 1, file);
            if (bytes < 0)
            {
                fclose(file);
                continue;
            }
            strcat(rcv, fbuf);
            // write(1, rcv, strlen(rcv));
            fclose(file);

            // send file content
            write(tempfd, rcv, RCVSIZE);
            // write(1, rcv, strlen(rcv));
            bzero(rcv, sizeof(rcv));
            write(ctrfd, "OK\n", strlen("OK\n"));

            // receive reply // 226
            bcnt += read(ctrfd, rsp, BUFSIZE);
            write(1, rsp, strlen(rsp));
            if (strncmp(rsp, "550", 3) == 0)
                continue;
            bzero(rsp, sizeof(rsp));
            write(ctrfd, "OK\n", strlen("OK\n"));

            // print total number of bytes received
            printf("OK. %d bytes is received.\n", bcnt);

            // close data connection
            close(tempfd);
        }
        else if (strcmp(comm, "TYPE") == 0)
        {
            bzero(rcv, sizeof(rcv));
            // receive reply // 201
            read(ctrfd, rcv, BUFSIZE);
            write(1, rcv, strlen(rcv));
            write(ctrfd, "OK\n", strlen("OK\n"));
        }
        else if (strcmp(comm, "PWD") == 0)
        {
            bzero(rcv, sizeof(rcv));
            // receive reply // 250
            read(ctrfd, rcv, BUFSIZE);
            write(1, rcv, strlen(rcv));
            write(ctrfd, "OK\n", strlen("OK\n"));
        }
        else if (strcmp(comm, "CWD") == 0)
        {
            bzero(rcv, sizeof(rcv));
            // receive reply // 250
            read(ctrfd, rcv, BUFSIZE);
            write(1, rcv, strlen(rcv));
            write(ctrfd, "OK\n", strlen("OK\n"));
        }
        else if (strcmp(comm, "CDUP") == 0)
        {
            bzero(rcv, sizeof(rcv));
            // receive reply // 250
            read(ctrfd, rcv, BUFSIZE);
            write(1, rcv, strlen(rcv));
            write(ctrfd, "OK\n", strlen("OK\n"));
        }
        else if (strcmp(comm, "MKD") == 0)
        {
            bzero(rcv, sizeof(rcv));
            // receive reply // 250
            read(ctrfd, rcv, BUFSIZE);
            write(1, rcv, strlen(rcv));
            write(ctrfd, "OK\n", strlen("OK\n"));
        }
        else if (strcmp(comm, "DELE") == 0)
        {
            bzero(rcv, sizeof(rcv));
            // receive reply // 250
            read(ctrfd, rcv, BUFSIZE);
            write(1, rcv, strlen(rcv));
            write(ctrfd, "OK\n", strlen("OK\n"));
        }
        else if (strcmp(comm, "RMD") == 0)
        {
            bzero(rcv, sizeof(rcv));
            // receive reply // 250
            read(ctrfd, rcv, BUFSIZE);
            write(1, rcv, strlen(rcv));
            write(ctrfd, "OK\n", strlen("OK\n"));
        }
        else if (strcmp(comm, "RNFR") == 0)
        {
            bzero(rcv, sizeof(rcv));
            // receive reply // 350
            read(ctrfd, rcv, BUFSIZE);
            write(1, rcv, strlen(rcv));
            write(ctrfd, "OK\n", strlen("OK\n"));

            // receive reply // 250
            read(ctrfd, rsp, BUFSIZE);
            write(1, rsp, strlen(rsp));
            write(ctrfd, "OK\n", strlen("OK\n"));
        }
        else if (strcmp(comm, "QUIT") == 0)
        {
            bzero(rcv, sizeof(rcv));
            read(ctrfd, rcv, BUFSIZE);
            write(1, rcv, strlen(rcv));
            write(ctrfd, "OK\n", strlen("OK\n"));
            break;
        }
    }

    close(ctrfd);
    close(datfd);
    close(tempfd);
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
        else if (arg == NULL)
        {
            printf("argument is required.\n");
            return -1;
        }

        // CWD
        strcat(cmd_buff, "CWD");
        strcat(cmd_buff, " ");
        strcat(cmd_buff, arg);
        return 0;
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
        strcat(cmd_buff, "RNFR ");
        char *arg = strtok(NULL, " ");
        strcat(cmd_buff, arg);
        strcat(cmd_buff, " ");

        // RNTO + arg2
        strcat(cmd_buff, "RNTO ");
        arg = strtok(NULL, " ");
        strcat(cmd_buff, arg);

        return 0;
    }
    else if (strcmp(cmd, "get") == 0)
    {
        // RETR
        strcpy(cmd_buff, "RETR ");
        char *arg = strtok(NULL, " ");
        if (arg == NULL)
        {
            printf("filename argument is required\n");
            return -1;
        }
        strcat(cmd_buff, arg);

        return 0;
    }
    else if (strcmp(cmd, "put") == 0)
    {
        // STOR
        strcpy(cmd_buff, "STOR ");
        char *arg = strtok(NULL, " ");
        if (arg == NULL)
        {
            printf("filename argument is required\n");
            return -1;
        }
        strcat(cmd_buff, arg);

        return 0;
    }
    else if (strcmp(cmd, "bin") == 0 || strcmp(cmd, "ascii") == 0 || strcmp(cmd, "type") == 0)
    {
        // TYPE
        if (strcmp(cmd, "bin") == 0)
        {
            strcpy(cmd_buff, "TYPE I");
            return 0;
        }
        else if (strcmp(cmd, "ascii") == 0)
        {
            strcpy(cmd_buff, "TYPE A");
            return 0;
        }
        else // type
        {
            strcpy(cmd_buff, "TYPE ");
            char *arg = strtok(NULL, " ");
            if (arg == NULL)
            {
                printf("type argument is required\n");
                return -1;
            }
            else if (strcmp(arg, "binary") == 0)
            {
                strcat(cmd_buff, "I");
                return 0;
            }
            else if (strcmp(arg, "ascii") == 0)
            {
                strcat(cmd_buff, "A");
                return 0;
            }
            else
            {
                printf("invalid type argument\n");
                return -1;
            }
        }
    }
    else if (strcmp(cmd, "quit") == 0)
    {
        // QUIT
        strcpy(cmd_buff, "QUIT");
        return 0;
    }

    return 0;
}

int log_in(int srvfd)
{
    int n;
    char user[32];
    char passwd[32];
    char *temp;
    char rsp[BUFSIZE];

    // try to login
    for (int i = 0; i < 3; i++)
    {
        // flush buffers
        bzero(user, 32);
        bzero(passwd, 32);
        bzero(rsp, sizeof(rsp));

        // get username
        write(1, "Name: ", strlen("Name: "));
        read(0, user, 32);

        // write username to server
        write(srvfd, user, strlen(user));
        read(srvfd, rsp, BUFSIZE);
        printf("%s", rsp);
        if (strncmp(rsp, "430", 3) == 0) // invalid username
            continue;
        bzero(rsp, sizeof(rsp));

        // get passwd
        temp = getpass("Password: ");
        strcpy(passwd, temp);

        // write passwd to server
        write(srvfd, passwd, strlen(passwd));
        read(srvfd, rsp, BUFSIZE);
        printf("%s", rsp);
        if (strncmp(rsp, "230", 3) == 0)
            return 0;
        bzero(rsp, sizeof(rsp));
    }

    // failed to login
    read(srvfd, rsp, BUFSIZE);
    printf("%s", rsp);
    return -1;
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

    // generate random port number 10001~60000
    srand(time(NULL));
    random_port = (rand() & 50000) + 10001;
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