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
#define RESULTBUFF 4096

// //////////////////////////////////////////////////////////////////////////////////////
// File Name : srv.c
// Date : 2024/05/01
// OS : Ubuntu 20.04.6 64-bit
//
// Arthor : Min Seok Choi
// Student ID : 2020202090
//
// Title: System Programming Assignment 2-1 (FTP server)
// Description: The purpose of Assignment 2-1 is to implement socket communication
//              ftp file system (server)
// //////////////////////////////////////////////////////////////////////////////////////

// compare for sort
int compare(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

// itos to make string
void int_to_str(int num, char *str)
{
    snprintf(str, 20, "%d", num);
}

// do NLST with param path, flags and result_buff
void NLST(char *path, int aflag, int lflag, char *result_buff)
{
    if (path == NULL || result_buff == NULL)
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

    if (dp == NULL && strcmp(path, "") != 0) // print file info
    {
        if (access(path, F_OK) == -1)
        {
            strcat(result_buff, "Error: No such file of directory\n");
            return;
        }
        else if (access(path, R_OK) == -1)
        {
            strcat(result_buff, "Error: cannot access\n");
            return;
        }

        if (lflag) // ls -l (file)
        {
            // get status
            stat(path, &statbuf);

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
            strcat(result_buff, perms);
            strcat(result_buff, " ");
            strcat(result_buff, ln_str);
            strcat(result_buff, " ");
            strcat(result_buff, pwd->pw_name);
            strcat(result_buff, " ");
            strcat(result_buff, grp->gr_name);
            strcat(result_buff, " ");
            strcat(result_buff, size_str);
            strcat(result_buff, " ");
            strcat(result_buff, time_str);
            strcat(result_buff, " ");
            strcat(result_buff, path);
        }
        else // ls (file)
        {
            strcat(result_buff, path);
        }
    }
    else // print directory entry info
    {
        // check permission if path is existing.
        if (strcmp(path, "") != 0)
        {
            if (access(path, F_OK) == -1)
            {
                bzero(result_buff, sizeof(result_buff));
                strcat(result_buff, "Error: No such file of directory\n");
                return;
            }
            else if (access(path, R_OK) == -1)
            {
                bzero(result_buff, sizeof(result_buff));
                strcat(result_buff, "Error: cannot access\n");
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
                strcat(result_buff, perms);
                strcat(result_buff, " ");
                strcat(result_buff, ln_str);
                strcat(result_buff, " ");
                strcat(result_buff, pwd->pw_name);
                strcat(result_buff, " ");
                strcat(result_buff, grp->gr_name);
                strcat(result_buff, " ");
                strcat(result_buff, size_str);
                strcat(result_buff, " ");
                strcat(result_buff, time_str);
                strcat(result_buff, " ");
                strcat(result_buff, namelist[i]);
                strcat(result_buff, (S_ISDIR(statbuf.st_mode)) ? "/\n" : "\n");
            }
        }
        else // ls + ls -a
        {
            for (int i = 0; i < count; i++)
            {
                // get status
                stat(namelist[i], &statbuf);

                // print directory entry
                strcat(result_buff, namelist[i]);
                if (((i + 1) % 5 == 0))
                    strcat(result_buff, "\n");
                else
                    strcat(result_buff, " ");
            }
            strcat(result_buff, "\n");
        }

        // close directory
        closedir(dp);

        // delete memory
        for (int i = 0; i < count; i++)
            free(namelist[i]);
        free(namelist);
    }
}

int cmd_process(char *buff, char *result_buff)
{
    if (buff == NULL || result_buff == NULL)
        return 0;

    char *line = strtok(buff, "\n");
    if (line == NULL)
        return 0;

    char *cmd = strtok(line, " ");
    if (strcmp(cmd, "NLST") == 0)
    {
        char *path = NULL;

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
                strcpy(result_buff, "Error: invalid option\n");
                return -1;
            }
            else if (strcmp(arg, "") != 0 && path == NULL)
            {
                if (path == NULL)
                    path = (char *)malloc(strlen(arg) + 1);

                strcpy(path, arg);
            }
            arg = strtok(NULL, " ");
        }

        if (path == NULL)
        {
            path = (char *)malloc(2);
            strcpy(path, "");
        }

        // do ls command
        NLST(path, aflag, lflag, result_buff);
        free(path);
        return 0;
    }
    else if (strcmp(cmd, "QUIT") == 0)
    {
        strcat(result_buff, "Program quit!!\n");
        return 0;
    }
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

int main(int argc, char **argv)
{
    struct sockaddr_in server_addr, client_addr;
    int socket_fd, client_fd;
    int len, len_out;
    int portno = 0;
    char buff[BUFFSIZE];
    char result_buff[RESULTBUFF];

    if (argc < 2)
    {
        write(2, "Port number is required\n", strlen("Port number is required\n"));
        return 0;
    }

    // get portno from argv
    portno = atoi(argv[1]);

    if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        write(1, "Server: Can't open stream socket\n", strlen("Server: Can't open stream socket\n"));
        return 0;
    }

    // set server address
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(portno);

    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        write(1, "Server: Can't bind local address\n", strlen("Server: Can't bind local address\n"));
        return 0;
    }

    listen(socket_fd, 5);
    while (1)
    {
        len = sizeof(client_addr);
        client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &len);

        // error handling
        if (client_fd < 0)
        {
            write(1, "Server: accept failed\n", strlen("Server: accept failed\n"));
            continue;
        }
        if (client_info(&client_addr) < 0)
        {
            write(2, "client_info() err!!\n", strlen("client_info() err!!\n"));
            continue;
        }

        // init buffer to zero
        bzero(buff, sizeof(buff));
        bzero(result_buff, sizeof(result_buff));

        // cmd process
        while ((len_out = read(client_fd, buff, BUFFSIZE)) > 0)
        {
            write(1, buff, len_out);
            write(1, "\n", 1);

            cmd_process(buff, result_buff);

            write(1, result_buff, strlen(result_buff));
            write(client_fd, result_buff, strlen(result_buff));
            if (strcmp(result_buff, "QUIT") == 0)
            {
                write(1, "QUIT\n", strlen("QUIT\n"));
                close(client_fd);
                break;
            }

            // clear buffer
            bzero(buff, sizeof(buff));
            bzero(result_buff, sizeof(result_buff));
        }
    }
    close(socket_fd);
    return 0;
}