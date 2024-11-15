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

// //////////////////////////////////////////////////////////////////////////////////////
// File Name : srv.c
// Date : 2024/05/20
// OS : Ubuntu 20.04.6 64-bit
//
// Arthor : Min Seok Choi
// Student ID : 2020202090
//
// Title: System Programming Assignment 3-1 (FTP server)
// Description: The purpose of Assignment 3-1 is to implement log-in sequence (server)
// //////////////////////////////////////////////////////////////////////////////////////

int check_ip(struct sockaddr_in *cliaddr);
int log_auth(int connfd);
int user_match(char *user, char *passwd);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Error: port number is required\n");
        return 0;
    }

    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t cliaddrlen = sizeof(cliaddr);
    int portno = atoi(argv[1]);

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("failed to create socket\n");
        return 0;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(portno);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        printf("failed to bind local address\n");
        close(listenfd);
        return 0;
    }

    listen(listenfd, 5);
    while (1)
    {
        // accept client
        if ((connfd = accept(listenfd, (struct sockaddr_in *)&cliaddr, &cliaddrlen)) < 0)
        {
            printf("failed to accept\n");
            close(connfd);
            continue;
        }

        // check ip whitelist
        if (check_ip(&cliaddr) == 0)
        {
            printf("** It is NOT authenticated client **\n");
            close(connfd);
            continue;
        }
        else
        {
            printf("** Client is connected **\n");
        }

        // login sequence
        if (log_auth(connfd) == 0)
        {
            printf("** Fail to log-in **\n");
            close(connfd);
            continue;
        }
        else
        {
            printf("** Success to log-in **\n");
            close(connfd);
        }

        close(connfd);
    }

    close(listenfd);
    return 0;
}

int check_ip(struct sockaddr_in *cliaddr)
{
    FILE *fp_checkIP = fopen("access.txt", "r");

    if (fp_checkIP == NULL)
    {
        printf("Error: Failed to open access.txt\n");
        return 0;
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(cliaddr->sin_addr), ip_str, INET_ADDRSTRLEN);

    printf("** Client is trying to connect **\n");
    printf("- IP:   %s\n", ip_str);
    printf("- port: %d\n", ntohs(cliaddr->sin_port));

    // search passwd file
    char line[INET_ADDRSTRLEN];
    while (fgets(line, sizeof(line), fp_checkIP) != NULL)
    {
        line[strcspn(line, "\n")] = 0;

        if (strcmp(ip_str, line) == 0)
        {
            fclose(fp_checkIP);
            return 1;
        }

        // parse ip
        char *token1 = strtok(ip_str, ".");
        char *token2 = strtok(line, ".");
        int match = 1;

        while (token1 != NULL && token2 != NULL)
        {
            if (strcmp(token1, "*") != 0 && strcmp(token1, token2) != 0)
            {
                match = 0;
                break; // compare with another line
            }
            token1 = strtok(NULL, ".");
            token2 = strtok(NULL, ".");
        }

        if (match == 1)
        {
            fclose(fp_checkIP);
            return 1; // ip matches
        }
    }

    fclose(fp_checkIP);
    return 0;
}

int log_auth(int connfd)
{
    char user[MAX_BUF];
    char passwd[MAX_BUF];
    int count = 1;

    for (int i = 0; i < 3; i++)
    {
        int n = 0;
        printf("** User is trying to log-in (%d/3) **\n", count);

        // read username and password from client
        n = read(connfd, user, MAX_BUF);
        if (n <= 0)
        {
            printf("Error reading username\n");
            return 0;
        }

        n = read(connfd, passwd, MAX_BUF);
        if (n <= 0)
        {
            printf("Error reading passwd\n");
            return 0;
        }

        if (user_match(user, passwd) == 1)
        {
            // login success
            write(connfd, "OK", MAX_BUF);
            return 1;
        }
        else
        {
            // login failed
            if (count >= 3) // failed for 3 times
            {
                write(connfd, "DISCONNECTION", MAX_BUF);
                close(connfd);
                return 0;
            }
            write(connfd, "FAIL", MAX_BUF);
            count++;
            continue;
        }
    }
}

int user_match(char *user, char *passwd)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("passwd", "r");
    if (fp == NULL)
    {
        printf("Failed to open passwd file\n");
        return 0;
    }

    // search access.txt
    while ((read = getline(&line, &len, fp)) != -1)
    {
        // Remove newline character from the line
        line[strcspn(line, "\n")] = 0;

        // parse username and passwd
        char *fuser = strtok(line, ":");
        char *fpasswd = strtok(NULL, ":");

        if (fuser == NULL || fpasswd == NULL)
        {
            continue; // compare with another line
        }

        if (strcmp(user, fuser) == 0 && strcmp(passwd, fpasswd) == 0)
        {
            free(line);
            fclose(fp);
            return 1; // Match found
        }
    }

    free(line);
    fclose(fp);
    return 0; // No match found
}
