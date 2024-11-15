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
// Date : 2024/05/26
// OS : Ubuntu 20.04.6 64-bit
//
// Arthor : Min Seok Choi
// Student ID : 2020202090
//
// Title: System Programming Assignment 3-2 (FTP server)
// Description: The purpose of Assignment 3-2 is to implement FTP system
//              with separate control connection and data connection (server)
// //////////////////////////////////////////////////////////////////////////////////////

int compare(const void *a, const void *b);
void NLST(char *path, int aflag, int lflag, char *rst_buff);
int cmd_process(char *buff, char *rst_buff);
void int_to_str(int num, char *str);
char *convert_str_to_addr(char *str, unsigned int *port);

int main(int argc, char **argv)
{
    int ctrfd, clifd, datfd, len;
    struct sockaddr_in ctraddr, cliaddr, dataddr;
    socklen_t ctrlen, clilen, datlen;
    char buf[BUFSIZE];
    char cmd[BUFSIZE];
    char rst[RSTSIZE];
    char *host_ip;
    int datport;

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

    // wait for client
    listen(ctrfd, 5);
    while (1)
    {
        // flush buffers
        bzero(buf, sizeof(buf));
        bzero(cmd, sizeof(cmd));
        bzero(rst, sizeof(rst));

        // accept control connection from client
        clilen = sizeof(cliaddr);
        clifd = accept(ctrfd, (struct sockaddr *)&cliaddr, &clilen);
        if (clifd < 0)
        {
            printf("failed to accept control connection\n");
            break;
        }

        // read hostport and perform ftp command
        while (1)
        {
            // flush buffers
            bzero(buf, sizeof(buf));
            bzero(cmd, sizeof(cmd));
            bzero(rst, sizeof(rst));
            len = 0;

            // read hostport from client
            read(clifd, buf, BUFSIZE);

            // check quick quit sequence
            if (strcmp(buf, "QUIT") == 0)
            {
                write(clifd, "221 Goodbye.\n", strlen("221 Goodbye.\n"));
                break;
            }

            host_ip = convert_str_to_addr(buf, &datport);
            printf("PORT %s\n", buf);

            // set data connection address
            datlen = sizeof(dataddr);
            memset(&dataddr, 0, datlen);
            dataddr.sin_family = AF_INET;
            dataddr.sin_addr.s_addr = inet_addr(host_ip);
            dataddr.sin_port = htons(datport);

            // create data connection socket and connect
            if ((datfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                printf("Error: failed to create socket\n");
                close(clifd);
                exit(1);
            }
            // connect data connection
            if (connect(datfd, (struct sockaddr *)&dataddr, datlen) < 0)
            {
                // send ERR to client
                printf("550 Failed to access\n");
                write(clifd, "550 Failed to access\n", strlen("550 Failed to access\n"));
                close(clifd);
                exit(1);
            }
            else
            {
                // send ACK to client
                printf("200 PORT command performed successfully.\n");
                write(clifd, "200 PORT command performed successfully.\n", strlen("200 PORT command performed successfully.\n"));
            }

            // receive ftp command and send ACK
            read(clifd, cmd, BUFSIZE);
            printf("%s\n", cmd);
            printf("150 Opening data connection for directory list.\n");
            write(clifd, "150 Opening data connection for directory list.\n", strlen("150 Opening data connection for directory list.\n"));

            // do cmd process
            len = cmd_process(cmd, rst);
            if (len < 0)
            {
                // send ERR to client
                write(clifd, "Error: cmd_process failed.\n", strlen("Error: cmd_process failed.\n"));
                close(clifd);
                close(datfd);
                exit(1);
            }

            // send ftp result and send ACK
            len = write(datfd, rst, strlen(rst));
            if (len < 0)
            {
                // send ERR to client
                printf("550 Failed transmission.\n");
                write(clifd, "550 Failed transmission.\n", strlen("550 Failed transmission.\n"));
                close(clifd);
                close(datfd);
                exit(1);
            }
            else
            {
                // send ACK to client
                printf("226 Complete transmission.\n");
                write(clifd, "226 Complete transmission.\n", strlen("226 Complete transmission.\n"));
            }

            close(datfd);
        }
    }

    // close socket descriptors
    if (ctrfd >= 0)
        close(ctrfd);
    if (datfd >= 0)
        close(datfd);
    if (clifd >= 0)
        close(clifd);

    return 0;
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
            if (((i + 1) % 5 == 0))
                strcat(rst_buff, "\n");
            else
                strcat(rst_buff, " ");
        }
        strcat(rst_buff, "\n");
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
                strcpy(rst_buff, "Error: invalid option\n");
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
        strcat(rst_buff, currdir);

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
            strcpy(rst_buff, "Error: path is required\n");
            return -1;
        }

        // change directory to path
        if (chdir(arg) != 0)
        {
            strcpy(rst_buff, "Error: directory not found\n");
            return -1;
        }

        // print current directory
        char currdir[64] = "";
        getcwd(currdir, 63);
        strcat(rst_buff, currdir);
        strcat(rst_buff, " ");
        strcat(rst_buff, "is currrnet directory\n");

        return 0;
    }
    else if (strcmp(cmd, "CDUP") == 0)
    {
        // change directory to parent directory
        if (chdir("..") != 0)
        {
            strcpy(rst_buff, "Error: directory not found\n");
            return -1;
        }

        // print current directory
        char currdir[64] = "";
        getcwd(currdir, 63);
        strcat(rst_buff, currdir);
        strcat(rst_buff, " ");
        strcat(rst_buff, "is currrnet directory\n");

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
                strcpy(rst_buff, "Error: cannot create directory\n");
                return -1;
            }

            // make directory
            if (mkdir(arg, 0755) == 0)
            {
                strcat(rst_buff, "MKD ");
                strcat(rst_buff, arg);
                strcat(rst_buff, "\n");
            }
            else
            {
                strcpy(rst_buff, "Error: failed to make directory\n");
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
                strcpy(rst_buff, "Error: file to delete is not existing\n");
                return -1;
            }

            // make directory
            if (unlink(arg) == 0)
            {
                strcat(rst_buff, "DELE ");
                strcat(rst_buff, arg);
                strcat(rst_buff, "\n");
            }
            else
            {
                strcpy(rst_buff, "Error: failed to delete\n");
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
                strcpy(rst_buff, "Error: file to remove is not existing\n");
                return -1;
            }

            // make directory
            if (rmdir(arg) == 0)
            {
                strcat(rst_buff, "DELE ");
                strcat(rst_buff, arg);
                strcat(rst_buff, "\n");
            }
            else
            {
                strcpy(rst_buff, "Error: failed to remove\n");
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
            strcpy(rst_buff, "Error: invalid arguments\n");
            return -1;
        }

        if (rename(arg1, arg2) == 0)
        {
            strcat(rst_buff, "RNFR ");
            strcat(rst_buff, arg1);
            strcat(rst_buff, " RNTO ");
            strcat(rst_buff, arg2);
            strcat(rst_buff, "\n");
        }
        else
        {
            strcpy(rst_buff, "Error: failed to change filename\n");
            return -1;
        }
    }
    else
    {
        strcat(rst_buff, "Error: invalid command\n");
        return -1;
    }

    return 0;
}

// itos to make string
void int_to_str(int num, char *str)
{
    snprintf(str, 20, "%d", num);
}
