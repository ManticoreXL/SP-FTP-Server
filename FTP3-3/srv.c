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
#define RSTSIZE 4096

// //////////////////////////////////////////////////////////////////////////////////////
// File Name : srv.c
// Date : 2024/06/04
// OS : Ubuntu 20.04.6 64-bit
//
// Arthor : Min Seok Choi
// Student ID : 2020202090
//
// Title: System Programming Assignment 3-3 (FTP server)
// Description: The purpose of Assignment 3-3 is to implement FTP system completely (server)
// //////////////////////////////////////////////////////////////////////////////////////

// functions for file processing
int compare(const void *a, const void *b);
void NLST(char *path, int aflag, int lflag, char *rst_buff);
int cmd_process(char *buff, char *rst_buff);
void int_to_str(int num, char *str);

// functions for access
int check_ip(struct sockaddr_in *cliaddr);
int log_auth(int clifd);
int user_find(char *user);
int user_match(char *user, char *passwd);
void welcome(char *buf);

// functions for data connection
char *convert_str_to_addr(char *str, unsigned int *port);

// functions for logging
void LOG_NO_CLI(char *buf);
void LOG(char *buf);

// user info
char guser[32];

// define control connection
int clifd;
struct sockaddr_in cliaddr;
socklen_t clilen;

// define data connection
int datfd, datport;
struct sockaddr_in dataddr;
socklen_t datlen;

// mode for selecting ascii and binary
int mode = 0;

int main(int argc, char **argv)
{
    int len = 0;
    int ctrfd;
    struct sockaddr_in ctraddr;
    socklen_t ctrlen;
    char buf[BUFSIZE];
    char cmd[BUFSIZE];
    char rst[RSTSIZE];
    char rsp[BUFSIZE];
    char *comm;
    char *host_ip;
    pid_t pid;

    if (argc < 2)
    {
        printf("port number is required\n");
        return 0;
    }

    // create control connection socket
    if ((ctrfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("failed to create control connection socket\n");
        return 0;
    }

    // set control connection address
    ctrlen = sizeof(ctraddr);
    memset(&ctraddr, 0, ctrlen);
    ctraddr.sin_family = AF_INET;
    ctraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    ctraddr.sin_port = htons(atoi(argv[1]));

    // bind local address
    if (bind(ctrfd, (struct sockaddr *)&ctraddr, ctrlen))
    {
        printf("can't bind local address\n");
        return 0;
    }

    // log server started
    LOG_NO_CLI("Server is started.\n");

    // wait for client
    listen(ctrfd, 5);
    while (1)
    {
        // flush buffers
        bzero(buf, sizeof(buf));
        bzero(cmd, sizeof(cmd));
        bzero(rst, sizeof(rst));
        bzero(rsp, sizeof(rsp));
        comm == NULL;

        // accept control connection from client
        clilen = sizeof(cliaddr);
        clifd = accept(ctrfd, (struct sockaddr *)&cliaddr, &clilen);
        if (clifd < 0)
        {
            printf("failed to accept control connection\n");
            break;
        }

        // check client ip
        int n = check_ip(&cliaddr);
        if (n < 0) // invalid ip
        {
            strcpy(rsp, "431 This client can't access. CLose the session.\n");
            printf("431 This client can't access. CLose the session.\n");
            write(clifd, rsp, strlen(rsp));
            LOG("LOG_FAIL\n");
            bzero(rsp, sizeof(rsp));
        }
        else // welcome
        {
            char time_str[64];
            time_t now;
            struct tm *timeinfo;

            // create current time information
            time(&now);
            timeinfo = localtime(&now);
            strftime(time_str, sizeof(time_str), "%a %b %d %H:%M:%S %Y", timeinfo);
            strcat(rsp, "sswlab.kw.ac.kr FTP server (version myftp [1.0] ");
            strcat(rsp, time_str);
            strcat(rsp, ")\n");
            write(clifd, rsp, strlen(rsp));
            LOG("LOG_IN\n");
            bzero(rsp, sizeof(rsp));
        }

        // log_in sequence
        if (log_auth(clifd) < 0)
            continue;

        // fork spot
        pid_t pid = fork();

        if (pid == 0)
        {
            while (1)
            {
                // flush buffers
                bzero(buf, sizeof(buf));
                bzero(cmd, sizeof(cmd));
                bzero(rst, sizeof(rst));
                bzero(rsp, sizeof(rsp));
                comm == NULL;
                len = 0;

                // ftp process
                read(clifd, cmd, BUFSIZE);
                printf("%s\n", cmd);
                LOG(cmd);
                LOG("\n");
                cmd_process(cmd, rst);

                // cmd-sensitive read/write
                comm = strtok(cmd, " ");
                if (strcmp(comm, "NLST") == 0 || strcmp(comm, "LIST") == 0)
                {
                    // read hostport from client
                    bzero(buf, sizeof(buf));
                    if (read(clifd, buf, BUFSIZE) < 0)
                    {
                        strcpy(rsp, "550 Failed to access.\n");
                        write(clifd, rsp, strlen(rsp));
                        write(1, rsp, strlen(rsp));
                        LOG(rsp);
                        bzero(rsp, sizeof(rsp));
                        continue;
                    }
                    bzero(rsp, sizeof(rsp));
                    strcat(rsp, "PORT ");
                    strcat(rsp, buf);
                    strcat(rsp, "\n");
                    write(1, rsp, strlen(rsp));
                    LOG(rsp);
                    bzero(rsp, sizeof(rsp));
                    host_ip = convert_str_to_addr(buf, &datport);
                    bzero(buf, sizeof(buf));

                    // set data connection address
                    datlen = sizeof(dataddr);
                    memset(&dataddr, 0, datlen);
                    dataddr.sin_family = AF_INET;
                    dataddr.sin_addr.s_addr = inet_addr(host_ip);
                    dataddr.sin_port = htons(datport);

                    // create data connection socket and connect
                    if ((datfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                    {
                        strcpy(rsp, "550 Failed to access.\n");
                        write(clifd, rsp, strlen(rsp));
                        write(1, rsp, strlen(rsp));
                        LOG(rsp);
                        bzero(rsp, sizeof(rsp));
                        continue;
                    }

                    if (connect(datfd, (struct sockaddr *)&dataddr, datlen) < 0)
                    {
                        strcpy(rsp, "550 Failed to access.\n");
                        write(clifd, rsp, strlen(rsp));
                        write(1, rsp, strlen(rsp));
                        LOG(rsp);
                        bzero(rsp, sizeof(rsp));
                        continue;
                    }

                    // send reply 200
                    strcpy(rsp, "200 PORT command performed successfully.\n");
                    write(clifd, rsp, strlen(rsp));
                    write(1, rsp, strlen(rsp));
                    LOG(rsp);
                    bzero(rsp, sizeof(rsp));
                    read(clifd, buf, BUFSIZE);
                    // write(1, buf, strlen(buf));
                    bzero(buf, sizeof(buf));

                    // send reply 150
                    strcpy(rsp, "150 Opening data connection for directory list.\n");
                    write(clifd, rsp, strlen(rsp));
                    write(1, rsp, strlen(rsp));
                    LOG(rsp);
                    bzero(rsp, sizeof(rsp));
                    read(clifd, buf, BUFSIZE);
                    // write(1, buf, strlen(buf));
                    bzero(buf, sizeof(buf));

                    // send ftp result
                    write(datfd, rst, RSTSIZE);
                    write(1, rst, strlen(rst));
                    read(clifd, buf, BUFSIZE);
                    // write(1, buf, strlen(buf));
                    bzero(buf, sizeof(buf));

                    // send reply 226
                    strcpy(rsp, "226 Complete Transmission.\n");
                    write(clifd, rsp, strlen(rsp));
                    write(1, rsp, strlen(rsp));
                    LOG(rsp);
                    bzero(rsp, sizeof(rsp));
                    read(clifd, buf, BUFSIZE);
                    // write(1, buf, strlen(buf));
                    bzero(buf, sizeof(buf));

                    // close data connection
                    close(datfd);
                }
                else if (strcmp(comm, "RETR") == 0)
                {
                    // read hostport from client
                    bzero(buf, sizeof(buf));
                    if (read(clifd, buf, BUFSIZE) < 0)
                    {
                        strcpy(rsp, "550 Failed to access.\n");
                        write(clifd, rsp, strlen(rsp));
                        write(1, rsp, strlen(rsp));
                        LOG(rsp);
                        bzero(rsp, sizeof(rsp));
                        continue;
                    }
                    bzero(rsp, sizeof(rsp));
                    strcat(rsp, "PORT ");
                    strcat(rsp, buf);
                    strcat(rsp, "\n");
                    write(1, rsp, strlen(rsp));
                    LOG(rsp);
                    bzero(rsp, sizeof(rsp));
                    host_ip = convert_str_to_addr(buf, &datport);
                    bzero(buf, sizeof(buf));

                    // set data connection address
                    datlen = sizeof(dataddr);
                    memset(&dataddr, 0, datlen);
                    dataddr.sin_family = AF_INET;
                    dataddr.sin_addr.s_addr = inet_addr(host_ip);
                    dataddr.sin_port = htons(datport);

                    // create data connection socket and connect
                    if ((datfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                    {
                        strcpy(rsp, "550 Failed to access.\n");
                        write(clifd, rsp, strlen(rsp));
                        write(1, rsp, strlen(rsp));
                        LOG(rsp);
                        bzero(rsp, sizeof(rsp));
                        continue;
                    }

                    if (connect(datfd, (struct sockaddr *)&dataddr, datlen) < 0)
                    {
                        strcpy(rsp, "550 Failed to access.\n");
                        write(clifd, rsp, strlen(rsp));
                        write(1, rsp, strlen(rsp));
                        LOG(rsp);
                        bzero(rsp, sizeof(rsp));
                        continue;
                    }

                    // send reply 200
                    strcpy(rsp, "200 PORT command performed successfully.\n");
                    write(clifd, rsp, strlen(rsp));
                    write(1, rsp, strlen(rsp));
                    LOG(rsp);
                    bzero(rsp, sizeof(rsp));
                    read(clifd, buf, BUFSIZE); // OK
                    // write(1, buf, strlen(buf));
                    bzero(buf, sizeof(buf));

                    // send reply 150
                    if (mode == 0)
                        strcat(rsp, "150 Opening binary mode data connection for get\n");
                    else if (mode == 1)
                        strcat(rsp, "150 Opening ascii mode data connection for get\n");
                    write(clifd, rsp, strlen(rsp));
                    write(1, rsp, strlen(rsp));
                    LOG(rsp);
                    bzero(rsp, sizeof(rsp));
                    read(clifd, buf, BUFSIZE); // OK
                    // write(1, buf, strlen(buf));
                    bzero(buf, sizeof(buf));

                    // send ftp result
                    write(datfd, rst, RSTSIZE);
                    write(1, "file contents sent.\n", strlen("file contents sent.\n"));
                    // write(1, rst, strlen(rst));
                    read(clifd, buf, BUFSIZE); // OK
                    // write(1, buf, strlen(buf));
                    bzero(buf, sizeof(buf));

                    // send reply 226
                    strcpy(rsp, "226 Complete Transmission.\n");
                    write(clifd, rsp, strlen(rsp));
                    write(1, rsp, strlen(rsp));
                    LOG(rsp);
                    bzero(rsp, sizeof(rsp));
                    read(clifd, buf, BUFSIZE); // OK
                    // write(1, buf, strlen(buf));
                    bzero(buf, sizeof(buf));

                    // close data connection
                    close(datfd);
                }
                else if (strcmp(comm, "STOR") == 0)
                {
                    // read hostport from client
                    bzero(buf, sizeof(buf));
                    if (read(clifd, buf, BUFSIZE) < 0)
                    {
                        strcpy(rsp, "550 Failed to access.\n");
                        write(clifd, rsp, strlen(rsp));
                        write(1, rsp, strlen(rsp));
                        LOG(rsp);
                        bzero(rsp, sizeof(rsp));
                        continue;
                    }
                    bzero(rsp, sizeof(rsp));
                    strcat(rsp, "PORT ");
                    strcat(rsp, buf);
                    strcat(rsp, "\n");
                    write(1, rsp, strlen(rsp));
                    LOG(rsp);
                    bzero(rsp, sizeof(rsp));
                    host_ip = convert_str_to_addr(buf, &datport);
                    bzero(buf, sizeof(buf));

                    // set data connection address
                    datlen = sizeof(dataddr);
                    memset(&dataddr, 0, datlen);
                    dataddr.sin_family = AF_INET;
                    dataddr.sin_addr.s_addr = inet_addr(host_ip);
                    dataddr.sin_port = htons(datport);

                    // create data connection socket and connect
                    if ((datfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                    {
                        strcpy(rsp, "550 Failed to access.\n");
                        write(clifd, rsp, strlen(rsp));
                        write(1, rsp, strlen(rsp));
                        LOG(rsp);
                        bzero(rsp, sizeof(rsp));
                        continue;
                    }

                    if (connect(datfd, (struct sockaddr *)&dataddr, datlen) < 0)
                    {
                        strcpy(rsp, "550 Failed to access.\n");
                        write(clifd, rsp, strlen(rsp));
                        write(1, rsp, strlen(rsp));
                        LOG(rsp);
                        bzero(rsp, sizeof(rsp));
                        continue;
                    }

                    // send reply 200
                    strcpy(rsp, "200 PORT command performed successfully.\n");
                    write(clifd, rsp, strlen(rsp));
                    write(1, rsp, strlen(rsp));
                    LOG(rsp);
                    bzero(rsp, sizeof(rsp));
                    read(clifd, buf, BUFSIZE);
                    // write(1, buf, strlen(buf));
                    bzero(buf, sizeof(buf));

                    // send reply 150
                    if (mode == 0)
                        strcat(rsp, "150 Opening binary mode data connection for get\n");
                    else if (mode == 1)
                        strcat(rsp, "150 Opening ascii mode data connection for get\n");
                    write(clifd, rsp, strlen(rsp));
                    write(1, rsp, strlen(rsp));
                    LOG(rsp);
                    bzero(rsp, sizeof(rsp));
                    read(clifd, buf, BUFSIZE); // OK
                    // write(1, buf, strlen(buf));
                    bzero(buf, sizeof(buf));

                    // receive file content
                    read(datfd, rst, RSTSIZE);
                    write(1, "file content received\n", strlen("file content received\n"));
                    read(clifd, buf, BUFSIZE); // OK
                    // write(1, buf, strlen(buf));
                    bzero(buf, sizeof(buf));

                    // file name argument is already existing
                    char *arg = strtok(rst, " ");
                    char *cont = strtok(NULL, "\0");
                    // write(1, cont, strlen(cont));
                    if (access(arg, F_OK) != -1)
                    {
                        bzero(rsp, sizeof(rsp));
                        strcpy(rsp, "550 Failed transmission.\n");
                        write(clifd, rsp, strlen(rsp));
                        continue;
                    }

                    // open file
                    FILE *file = (mode == 0) ? fopen(arg, "wb") : fopen(arg, "w");
                    if (file == NULL)
                    {
                        printf("failed to create file.\n");
                        continue;
                    }
                    size_t bwrite = strlen(cont);
                    size_t bwritten = fwrite(cont, 1, bwrite, file);
                    if (bwrite != bwritten)
                    {
                        printf("failed to write file\n");
                        fclose(file);
                        continue;
                    }
                    fclose(file);

                    // send reply 226
                    strcpy(rsp, "226 Complete Transmission.\n");
                    write(clifd, rsp, strlen(rsp));
                    write(1, rsp, strlen(rsp));
                    LOG(rsp);
                    bzero(rsp, sizeof(rsp));
                    read(clifd, buf, BUFSIZE); // OK
                    // write(1, buf, strlen(buf));
                    bzero(buf, sizeof(buf));

                    // close data connection
                    close(datfd);
                }
                else if (strcmp(comm, "TYPE") == 0)
                {
                    printf("%s", rst);
                    write(clifd, rst, strlen(rst));
                    read(clifd, buf, BUFSIZE);
                    // write(1, buf, strlen(buf));
                }
                else if (strcmp(comm, "PWD") == 0)
                {
                    printf("%s", rst);
                    write(clifd, rst, strlen(rst));
                    read(clifd, buf, BUFSIZE);
                    // write(1, buf, strlen(buf));
                }
                else if (strcmp(comm, "CWD") == 0)
                {
                    printf("%s", rst);
                    write(clifd, rst, strlen(rst));
                    read(clifd, buf, BUFSIZE);
                    // write(1, buf, strlen(buf));
                }
                else if (strcmp(comm, "CDUP") == 0)
                {
                    printf("%s", rst);
                    write(clifd, rst, strlen(rst));
                    read(clifd, buf, BUFSIZE);
                    // write(1, buf, strlen(buf));
                }
                else if (strcmp(comm, "MKD") == 0)
                {
                    printf("%s", rst);
                    write(clifd, rst, strlen(rst));
                    read(clifd, buf, BUFSIZE);
                    // write(1, buf, strlen(buf));
                }
                else if (strcmp(comm, "DELE") == 0)
                {
                    printf("%s", rst);
                    write(clifd, rst, strlen(rst));
                    read(clifd, buf, BUFSIZE);
                    // write(1, buf, strlen(buf));
                }
                else if (strcmp(comm, "RMD") == 0)
                {
                    printf("%s", rst);
                    write(clifd, rst, strlen(rst));
                    read(clifd, buf, BUFSIZE);
                    // write(1, buf, strlen(buf));
                }
                else if (strcmp(comm, "RNFR") == 0)
                {
                    printf("%s", rst);
                    write(clifd, rst, strlen(rst));
                    read(clifd, buf, BUFSIZE);
                    // write(1, buf, strlen(buf));
                    bzero(buf, sizeof(buf));

                    strcat(rsp, "250 RNTO command performed successfully.\n");
                    write(clifd, rsp, strlen(rsp));
                    read(clifd, buf, BUFSIZE);
                    // write(1, buf, strlen(buf));
                    bzero(buf, sizeof(buf));
                }
                else if (strcmp(comm, "QUIT") == 0)
                {
                    printf("%s", rst);
                    write(clifd, rst, strlen(rst));
                    read(clifd, buf, BUFSIZE);
                    // write(1, buf, strlen(buf));
                    close(clifd);
                    break;
                }
            }
        }
    }

    // close socket descriptors
    close(ctrfd);
    close(datfd);
    close(clifd);

    return 0;
}

// compare for sort
int compare(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

// do NLST with param path, flags and rst_buff
void NLST(char *path, int aflag, int lflag, char *rst_buff)
{
    if (path == NULL || rst_buff == NULL)
        return;

    DIR *dp;
    struct dirent *entry = 0;
    struct stat statbuf;
    struct passwd *pwd;
    struct group *grp;
    struct tm *timeinfo;
    char **namelist = NULL;
    int count = 0;

    if (strcmp(path, "") == 0)
        dp = opendir("./");
    else
        dp = opendir(path);

    // check permission if path is existing.
    if (strcmp(path, "") != 0)
    {
        stat(path, &statbuf);

        if (S_ISDIR(statbuf.st_mode) == 0)
        {
            strcpy(rst_buff, "Error: directory is required\n");
            return;
        }
        else if (access(path, F_OK) == -1)
        {
            strcpy(rst_buff, "Error: No such directory\n");
            return;
        }
        else if (access(path, R_OK) == -1)
        {
            strcpy(rst_buff, "Error: cannot access\n");
            return;
        }
    }

    // get directory entries
    while ((entry = readdir(dp)) != 0)
    {
        if (!aflag && (entry->d_name[0] == '.'))
            continue;
        namelist = realloc(namelist, (count + 1) * sizeof(char *));
        namelist[count] = strdup(entry->d_name);
        count++;
    }

    // sort by ASCII ascending order
    qsort(namelist, count, sizeof(char *), compare);

    // print directory entires
    if (lflag) // ls -l + ls -al
    {
        for (int i = 0; i < count; i++)
        {
            stat(namelist[i], &statbuf);

            // get file permissions
            char perms[11];
            perms[0] = (S_ISDIR(statbuf.st_mode)) ? 'd' : '-';
            perms[1] = (statbuf.st_mode & S_IRUSR) ? 'r' : '-';
            perms[2] = (statbuf.st_mode & S_IWUSR) ? 'w' : '-';
            perms[3] = (statbuf.st_mode & S_IXUSR) ? 'x' : '-';
            perms[4] = (statbuf.st_mode & S_IRGRP) ? 'r' : '-';
            perms[5] = (statbuf.st_mode & S_IWGRP) ? 'w' : '-';
            perms[6] = (statbuf.st_mode & S_IXGRP) ? 'x' : '-';
            perms[7] = (statbuf.st_mode & S_IROTH) ? 'r' : '-';
            perms[8] = (statbuf.st_mode & S_IWOTH) ? 'w' : '-';
            perms[9] = (statbuf.st_mode & S_IXOTH) ? 'x' : '-';
            perms[10] = '\0';

            // get number of hard link
            char ln_str[20];
            int_to_str((int)statbuf.st_nlink, ln_str);

            // get owner name
            if ((pwd = getpwuid(statbuf.st_uid)) == NULL)
            {
                write(1, "Error: failed to get owner name\n", strlen("Error: failed to get owner name\n"));
                return;
            }

            // get group name
            if ((grp = getgrgid(statbuf.st_gid)) == NULL)
            {
                write(1, "Error: failed to get group name\n", strlen("Error: failed to get group name\n"));
                return;
            }

            // get size string
            char size_str[20];
            int_to_str((int)statbuf.st_size, size_str);

            // get time string
            timeinfo = localtime(&statbuf.st_mtime);
            char time_str[32];
            strftime(time_str, 32, "%b %d %H:%M", timeinfo);

            // print file info
            strcat(rst_buff, perms);
            strcat(rst_buff, " ");
            strcat(rst_buff, ln_str);
            strcat(rst_buff, " ");
            strcat(rst_buff, pwd->pw_name);
            strcat(rst_buff, " ");
            strcat(rst_buff, grp->gr_name);
            strcat(rst_buff, " ");
            strcat(rst_buff, size_str);
            strcat(rst_buff, " ");
            strcat(rst_buff, time_str);
            strcat(rst_buff, " ");
            strcat(rst_buff, namelist[i]);
            strcat(rst_buff, (S_ISDIR(statbuf.st_mode)) ? "/\n" : "\n");
        }
    }
    else // ls + ls -a
    {
        for (int i = 0; i < count; i++)
        {
            // get status
            stat(namelist[i], &statbuf);

            // print directory entry
            strcat(rst_buff, namelist[i]);
            strcat(rst_buff, "\n");
        }
    }

    // close directory
    closedir(dp);

    // delete memory
    for (int i = 0; i < count; i++)
        free(namelist[i]);
    free(namelist);
}

int cmd_process(char *buff, char *rst_buff)
{
    if (buff == NULL || rst_buff == NULL)
        return 0;

    // get line
    char *line = strtok(buff, "\n");
    if (line == NULL)
        return 0;

    // get command
    char *cmd = strtok(line, " ");

    // command process
    if (strcmp(cmd, "NLST") == 0) // name list
    {
        char *path = 0;

        int aflag = 0;
        int lflag = 0;

        // get option and path
        char *arg = strtok(NULL, " ");
        while (arg != NULL)
        {
            if (strcmp(arg, "-al") == 0 || strcmp(arg, "-la") == 0)
            {
                aflag++;
                lflag++;
            }
            else if (strcmp(arg, "-a") == 0)
                aflag++;
            else if (strcmp(arg, "-l") == 0)
                lflag++;
            else if (arg[0] == '-')
            {
                strcpy(rst_buff, "550 invalid option\n");
                return -1;
            }
            else if (strcmp(arg, "") != 0 && path == NULL)
            {
                if (path == NULL)
                    path = (char *)malloc(strlen(arg) + 1);

                strcpy(path, arg);
            }

            // get next argument
            arg = strtok(NULL, " ");
        }

        if (path == NULL)
        {
            path = (char *)malloc(2);
            strcpy(path, "");
        }

        // do ls command
        NLST(path, aflag, lflag, rst_buff);
        free(path);
        return 0;
    }
    else if (strcmp(cmd, "PWD") == 0)
    {
        char currdir[64] = "";
        getcwd(currdir, 63);
        strcat(rst_buff, "257 \"");
        strcat(rst_buff, currdir);
        strcat(rst_buff, "\" is curent directory.\n");
        LOG(rst_buff);
        return 0;
    }
    else if (strcmp(cmd, "LIST") == 0) // list all directory entires
    {
        char *path = NULL;

        // get path
        char *arg = strtok(NULL, " ");
        while (arg != NULL)
        {
            if (strcmp(arg, "") != 0 && path == NULL)
            {
                if (path == NULL)
                    path = (char *)malloc(strlen(arg) + 1);

                strcpy(path, arg);
            }

            // get next argument
            arg = strtok(NULL, " ");
        }

        // no path
        if (path == NULL)
        {
            path = (char *)malloc(2);
            strcpy(path, "");
        }

        // do ls -al command
        NLST(path, 1, 1, rst_buff);
        free(path);
        return 0;
    }
    else if (strcmp(cmd, "CWD") == 0) // change working directory
    {
        // get path
        char *arg = strtok(NULL, " ");

        if (arg == NULL)
        {
            strcpy(rst_buff, "550 path is required\n");
            LOG(rst_buff);
            return -1;
        }

        // change directory to path
        if (chdir(arg) != 0)
        {
            strcat(rst_buff, "550 ");
            strcat(rst_buff, arg);
            strcat(rst_buff, ": Can't find such file or directory.\n");
            LOG(rst_buff);
            return -1;
        }

        // complete message
        strcpy(rst_buff, "250 CWD command performed successfully.\n");
        LOG(rst_buff);
        return 0;
    }
    else if (strcmp(cmd, "CDUP") == 0)
    {
        // change directory to parent directory
        if (chdir("..") != 0)
        {
            strcat(rst_buff, "550 ..: Can't find such file or directory.\n");
            LOG(rst_buff);
            return -1;
        }

        // complete message
        strcpy(rst_buff, "250 CDUP command performed successfully.\n");
        LOG(rst_buff);
        return 0;
    }
    else if (strcmp(cmd, "MKD") == 0) // make directory
    {
        // get arguments
        char *arg = strtok(NULL, " ");
        while (arg != NULL)
        {
            if (access(arg, F_OK) != -1)
            {
                strcat(rst_buff, "550 ");
                strcat(rst_buff, arg);
                strcat(rst_buff, ": can't create directory.\n");
                LOG(rst_buff);
                return -1;
            }

            // make directory
            if (mkdir(arg, 0755) == 0)
            {
                strcat(rst_buff, "250 MKD command performed successfully.\n");
                LOG(rst_buff);
            }
            else
            {
                strcat(rst_buff, "550 ");
                strcat(rst_buff, arg);
                strcat(rst_buff, ": can't create directory.\n");
                LOG(rst_buff);
                return -1;
            }

            // get next argument
            arg = strtok(NULL, " ");
        }

        return 0;
    }
    else if (strcmp(cmd, "DELE") == 0) // delete file
    {
        // get arguments
        char *arg = strtok(NULL, " ");
        while (arg != NULL)
        {
            if (access(arg, F_OK) == -1)
            {
                strcat(rst_buff, "550 ");
                strcat(rst_buff, arg);
                strcat(rst_buff, ": can't delete directory.\n");
                LOG(rst_buff);
                return -1;
            }

            // make directory
            if (unlink(arg) == 0)
            {
                strcat(rst_buff, "250 DELE command performed successfully.\n");
                LOG(rst_buff);
            }
            else
            {
                strcat(rst_buff, "550 ");
                strcat(rst_buff, arg);
                strcat(rst_buff, ": can't delete directory.\n");
                LOG(rst_buff);
                return -1;
            }

            // get next argument
            arg = strtok(NULL, " ");
        }

        return 0;
    }
    else if (strcmp(cmd, "RMD") == 0) // remove directory
    {
        // get arguments
        char *arg = strtok(NULL, " ");
        while (arg != NULL)
        {
            if (access(arg, F_OK) == -1)
            {
                strcat(rst_buff, "550 ");
                strcat(rst_buff, arg);
                strcat(rst_buff, ": can't remove directory.\n");
                LOG(rst_buff);
                return -1;
            }

            // make directory
            if (rmdir(arg) == 0)
            {
                strcat(rst_buff, "250 RMD command performed successfully.\n");
                LOG(rst_buff);
            }
            else
            {
                strcat(rst_buff, "550 ");
                strcat(rst_buff, arg);
                strcat(rst_buff, ": can't remove directory.\n");
                LOG(rst_buff);
                return -1;
            }

            // get next argument
            arg = strtok(NULL, " ");
        }

        return 0;
    }
    else if (strcmp(cmd, "RNFR") == 0) // rename
    {
        char *arg1 = strtok(NULL, " ");
        char *cmd2 = strtok(NULL, " ");
        char *arg2 = strtok(NULL, " ");

        if (arg1 == NULL || cmd2 == NULL || arg2 == NULL)
        {
            strcpy(rst_buff, "550 invalid arguments\n");
            return -1;
        }

        if (rename(arg1, arg2) == 0)
        {
            strcat(rst_buff, "350 File exists, ready to rename.\n");
            LOG(rst_buff);
        }
        else
        {
            strcat(rst_buff, "550 ");
            strcat(rst_buff, arg1);
            strcat(rst_buff, ": Can't find such file or directory.\n");
            LOG(rst_buff);
            return -1;
        }
    }
    else if (strcmp(cmd, "RETR") == 0)
    {
        char *arg = strtok(NULL, " ");
        FILE *file;
        if (arg == NULL)
        {
            strcpy(rst_buff, "550 invalid arugment\n");
            return -1;
        }

        // open file
        if (mode == 0) // binary mode
            file = fopen(arg, "rb");
        else if (mode == 1) // ascii mode
            file = fopen(arg, "r");

        if (file == NULL)
        {
            strcpy(rst_buff, "550 file not found\n");
            return -1;
        }

        // read file
        size_t bytes = fread(rst_buff, 1, RSTSIZE - 1, file);
        if (bytes < 0)
        {
            strcpy(rst_buff, "550 error reading file\n");
            fclose(file);
            return -1;
        }
    }
    else if (strcmp(cmd, "STOR") == 0)
    {
        char *arg = strtok(NULL, " ");
        if (arg == NULL)
        {
            strcpy(rst_buff, "550 invalid arugment\n");
            return -1;
        }
    }
    else if (strcmp(cmd, "TYPE") == 0)
    {
        char *arg = strtok(NULL, " ");
        if (arg == NULL)
        {
            strcat(rst_buff, "502 Type doesn't set.\n");
            LOG(rst_buff);
            return -1;
        }

        if (strcmp(arg, "I") == 0) // binary mode
        {
            mode = 0;
            strcpy(rst_buff, "201 Type set to I.\n");
            LOG(rst_buff);
            return 0;
        }
        else if (strcmp(arg, "A") == 0) // ascii mode
        {
            mode = 1;
            strcpy(rst_buff, "201 Type set to A.\n");
            LOG(rst_buff);
            return 0;
        }
    }
    else if (strcmp(cmd, "QUIT") == 0)
    {
        strcpy(rst_buff, "221 Goodbye.\n");
        return 0;
    }
    else
    {
        strcat(rst_buff, "550 invalid command\n");
        LOG(rst_buff);
        return -1;
    }

    return 0;
}

// itos to make string
void int_to_str(int num, char *str)
{
    snprintf(str, 20, "%d", num);
}

int check_ip(struct sockaddr_in *cliaddr)
{
    FILE *fp_checkIP = fopen("access.txt", "r");

    if (fp_checkIP == NULL)
    {
        printf("Error: Failed to open access.txt\n");
        return -1;
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(cliaddr->sin_addr), ip_str, INET_ADDRSTRLEN);

    // printf("** Client is trying to connect **\n");
    // printf("- IP:   %s\n", ip_str);
    // printf("- port: %d\n", ntohs(cliaddr->sin_port));

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
    return -1;
}

int log_auth(int clifd)
{
    char user[32];
    char passwd[32];
    char rsp[BUFSIZE];
    char buf[BUFSIZE];
    int i = 0;

    // client try to login
    for (i = 0; i < 3; i++)
    {
        // flush buffers
        bzero(rsp, sizeof(rsp));
        bzero(buf, sizeof(buf));
        bzero(user, 32);
        bzero(passwd, 32);

        // read username from client
        read(clifd, user, 32);
        user[strcspn(user, "\n")] = 0;
        strcpy(guser, user); // record username
        strcat(buf, "USER ");
        strcat(buf, user);
        strcat(buf, "\n");
        LOG(buf);
        bzero(buf, sizeof(buf));
        if (user_find(user) < 0)
        {
            // user doesn't exist
            bzero(rsp, sizeof(rsp));
            strcat(rsp, "430 Invalid username or passwd\n");
            write(clifd, rsp, strlen(rsp));
            write(1, rsp, strlen(rsp));
            LOG(rsp);
            bzero(rsp, sizeof(rsp));
            continue;
        }
        else
        {
            // user is in white list
            bzero(rsp, sizeof(rsp));
            strcat(rsp, "331 Passwd required for ");
            strcat(rsp, user);
            strcat(rsp, "\n");
            write(clifd, rsp, strlen(rsp));
            write(1, rsp, strlen(rsp));
            LOG(rsp);
            bzero(rsp, sizeof(rsp));
        }

        // read passwd from
        read(clifd, passwd, 32);
        passwd[strcspn(passwd, "\n")] = 0;
        strcat(buf, "PASS ");
        strcat(buf, passwd);
        strcat(buf, "\n");
        LOG(buf);
        bzero(buf, sizeof(buf));
        if (user_match(user, passwd) < 0)
        {
            // failed to login
            bzero(rsp, sizeof(rsp));
            strcat(rsp, "430 Invalid username or passwd\n");
            write(clifd, rsp, strlen(rsp));
            write(1, rsp, strlen(rsp));
            LOG(rsp);
            bzero(rsp, sizeof(rsp));
            continue;
        }
        else
        {
            // success to login
            bzero(rsp, sizeof(rsp));
            strcat(rsp, "230 User ");
            strcat(rsp, user);
            strcat(rsp, " logged in.\n");
            write(clifd, rsp, strlen(rsp));
            write(1, rsp, strlen(rsp));
            LOG(rsp);
            bzero(rsp, sizeof(rsp));
            return 1;
        }
    }
    // failed to login for 3 times
    bzero(rsp, sizeof(rsp));
    strcpy(rsp, "530 Failed to log-in");
    write(clifd, rsp, strlen(rsp));
    LOG(rsp);
    return -1;
}

int user_find(char *user)
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
        // parse username and passwd
        char *fuser = strtok(line, ":");

        if (fuser == NULL)
            continue; // compare with another line

        if (strcmp(user, fuser) == 0)
        {
            free(line);
            fclose(fp);
            return 1; // match found
        }
    }

    free(line);
    fclose(fp);
    return -1; // no match found
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
    return -1; // No match found
}

void welcome(char *buf)
{
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);

    // get time info
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%a %b %d %H:%M:%S KST %Y", tm_now);

    // open motd file
    FILE *file = fopen("motd", "r");
    if (file == NULL)
    {
        printf("failed to open motd file\n");
        return;
    }

    // read motd file
    char welcome[256];
    if (fgets(welcome, sizeof(welcome), file) == NULL)
    {
        printf("failed to read motd file\n");
        fclose(file);
        return;
    }

    // copy welcome message to buf
    bzero(buf, sizeof(buf));
    // snprintf(buf, sizeof(buf), welcome, time_str);
    strcpy(buf, welcome);
}

char *convert_str_to_addr(char *str, unsigned int *port)
{
    char *addr;
    struct in_addr ip_struct;
    unsigned ip_bytes[4];
    unsigned porthi, portlo;

    // split hostport string
    sscanf(str, "%u, %u, %u, %u, %u, %u", &ip_bytes[0], &ip_bytes[1], &ip_bytes[2], &ip_bytes[3], &porthi, &portlo);

    // reconstruct ip address
    ip_struct.s_addr = (ip_bytes[0] << 24) | (ip_bytes[1] << 16) | (ip_bytes[2] << 8) | ip_bytes[3];
    ip_struct.s_addr = htonl(ip_struct.s_addr);

    // reconstruct port number
    *port = (porthi << 8) | portlo;

    // convert ip into string
    addr = inet_ntoa(ip_struct);
    if (addr == NULL)
        return NULL;

    // allocate memory for the result and copy the ip address string
    addr = strdup(addr);
    if (addr == NULL)
        return NULL;

    // return ip address string
    return addr;
}

void LOG_NO_CLI(char *buf)
{
    const char *logfile = "logfile";
    char time_str[128];
    time_t now;
    struct tm *timeinfo;

    FILE *fp = fopen(logfile, "a");
    if (fp == NULL)
    {
        printf("Error opening logfile");
        return;
    }

    // create current time information
    time(&now);
    timeinfo = localtime(&now);
    strftime(time_str, sizeof(time_str), "%a %b %d %H:%M:%S %Y", timeinfo);

    fprintf(fp, "%s %s", time_str, buf);
    fclose(fp);
}

void LOG(char *buf)
{
    const char *logfile = "logfile";
    char time_str[128];
    char ip_str[128];
    char log_msg[BUFSIZE];
    time_t now;
    struct tm *timeinfo;

    // create current time information
    time(&now);
    timeinfo = localtime(&now);
    strftime(time_str, sizeof(time_str), "%a %b %d %H:%M:%S %Y", timeinfo);

    // get ip address string
    char *temp = inet_ntoa(cliaddr.sin_addr);
    strcpy(ip_str, temp);

    FILE *fp = fopen(logfile, "a");
    if (fp == NULL)
    {
        printf("Error opening logfile");
        return;
    }

    // unknown client name
    if (strcmp(guser, "") == 0)
        strcpy(guser, "client");

    // logging
    snprintf(log_msg, sizeof(log_msg), "%s [%s:%d] %s %s", time_str, ip_str, cliaddr.sin_port, guser, buf);
    log_msg[strlen(log_msg)] == '\0';
    fprintf(fp, "%s", log_msg);
    fclose(fp);
}