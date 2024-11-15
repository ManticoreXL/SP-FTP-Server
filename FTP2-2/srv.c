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
// File Name : srv.c
// Date : 2024/05/06
// OS : Ubuntu 20.04.6 64-bit
//
// Arthor : Min Seok Choi
// Student ID : 2020202090
//
// Title: System Programming Assignment 2-2 (FTP server)
// Description: The purpose of Assignment 2-2 is to implement socket communication
//              ftp file system with fork() (server)
// //////////////////////////////////////////////////////////////////////////////////////

int client_info(struct sockaddr_in *client_addr);
void sh_chld(int signum);
void sh_alrm(int signum);

int main(int argc, char **argv)
{
    char buff[BUF_SIZE];
    int n;
    struct sockaddr_in server_addr, client_addr;
    int server_fd, client_fd;
    int len;
    int port;

    // applying signal handler (sh_alrm) for SIGALRM
    signal(SIGCHLD, sh_chld);
    // applying signal hanlder (sh_chld) for SIGCHLD
    signal(SIGALRM, sh_alrm);

    server_fd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    // wait for client
    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 5);
    while (1)
    {
        pid_t pid;
        len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);

        pid = fork();

        if (pid == 0) // child process
        {
            // print process id
            printf("Child Process ID : %d\n", (int)getpid());

            bzero(buff, sizeof(buff));
            while ((len = read(client_fd, buff, BUF_SIZE)) > 0)
            {
                if (strcmp(buff, "QUIT\n") == 0 || strcmp(buff, "QUIT") == 0)
                {
                    // send alarm (QUIT)
                    alarm(1);
                }
                else
                {
                    // send echo to client
                    write(client_fd, buff, len);
                }
                // flush buffer
                bzero(buff, sizeof(buff));
            }
        }
        else // parent process
        {
            // print client info
            client_info(&client_addr);
        }
        close(client_fd);
    }

    return 0;
}

// print client info
int client_info(struct sockaddr_in *client_addr)
{
    if (client_addr == NULL)
        return -1;

    char ip_str[26];
    inet_ntop(AF_INET, &(client_addr->sin_addr), ip_str, INET_ADDRSTRLEN);

    char portno_str[32];
    sprintf(portno_str, "%d", ntohs(client_addr->sin_port));

    write(1, "==========Client info===========\n", strlen("==========Client info===========\n"));
    write(1, "client IP: ", strlen("client IP: "));
    write(1, ip_str, strlen(ip_str));
    write(1, "\n\n", 2);
    write(1, "client port: ", strlen("client port: "));
    write(1, portno_str, strlen(portno_str));
    write(1, "\n================================\n", strlen("\n================================\n"));

    return 0;
}

// signal child handler
void sh_chld(int signum)
{
    printf("Status of Child process was changed.\n");
    wait(NULL);
}

// signal alarm handler
void sh_alrm(int signum)
{
    printf("Child Process(PID: %d) will be terminated.\n", signum);
    exit(1);
}