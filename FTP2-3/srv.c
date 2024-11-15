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
// File Name : srv.c
// Date : 2024/05/13
// OS : Ubuntu 20.04.6 64-bit
//
// Arthor : Min Seok Choi
// Student ID : 2020202090
//
// Title: System Programming Assignment 2-3 (FTP server)
// Description: The purpose of Assignment 2-3 is to implement socket communication
//              ftp file system with fork(). (server)
// //////////////////////////////////////////////////////////////////////////////////////

#define BUFSIZE 1024
#define RSTSIZE 4096
#define INITCAPA 8

struct cli
{
    pid_t pid;
    int port;
    time_t start_time;
    time_t service_time;
    int cli_fd;
};

struct cli *cli_list = NULL;
int cli_count = 0;
int capacity = 0;

void resize_list();

char buff[BUFSIZE];
char rst_buff[RSTSIZE];
int server_fd, client_fd;

int client_info(struct sockaddr_in *client_addr);
int compare(const void *a, const void *b);
int cmp_pid(const void *a, const void *b);
int cmd_process(char *buff, char *rst_buff);
void int_to_str(int num, char *str);

void sh_chld(int signum);
void sh_alrm(int signum);
void sh_intr(int signum);

int main(int argc, char **argv)
{
    struct sockaddr_in server_addr, client_addr;
    int len;

    if (argc < 2)
    {
        printf("Port number is required\n");
        return 0;
    }

    // applying signal handler (sh_alrm) for SIGALRM
    signal(SIGCHLD, sh_chld);
    // applying signal hanlder (sh_chld) for SIGCHLD
    signal(SIGALRM, sh_alrm);
    // applying signal handler (sh_intr) for SIGINT
    signal(SIGINT, sh_intr);

    // init cli_list
    resize_list();

    // create socket
    if ((server_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Can't open stream socket\n");
        return 0;
    }

    // set server address
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    // bind local address
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Error: Can't bind local address\n");
        return 0;
    }

    // wait for client
    listen(server_fd, 5);
    while (1)
    {
        pid_t pid;
        len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&len);

        if (client_fd < 0)
        {
            printf("Error: accept failed\n");
            continue;
        }

        pid = fork();
        if (pid < 0)
        {
            printf("Error: fork error\n");
            break;
        }

        if (pid == 0) // child process
        {
            // print process id
            printf("Child Process ID : %d\n", (int)getpid());

            // init buffs
            memset(buff, 0, strlen(buff));
            memset(rst_buff, 0, strlen(rst_buff));

            // read from client
            while ((len = read(client_fd, buff, BUFSIZE)) > 0)
            {
                // server process
                printf("%s [%d]\n", buff, getpid());

                // quit
                if (strcmp(buff, "QUIT") == 0)
                    break;

                cmd_process(buff, rst_buff);

                // printf("%s\n", rst_buff);
                write(client_fd, rst_buff, strlen(rst_buff));

                // flush buffer
                memset(buff, 0, strlen(buff));
                memset(rst_buff, 0, strlen(rst_buff));
            }
            close(client_fd);
            exit(0);
        }
        else // parent process
        {
            // print client info
            client_info(&client_addr);

            // add child process to cli_list
            cli_list[cli_count].pid = pid;
            cli_list[cli_count].port = ntohs(client_addr.sin_port);
            cli_list[cli_count].start_time = time(NULL);
            cli_list[cli_count].cli_fd = client_fd;
            cli_count++;

            // extend list if it's full
            if (cli_count == capacity)
                resize_list();

            // init alarm
            alarm(10);
        }
    }
    close(server_fd);
    free(cli_list);
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
    for (int i = 0; i < cli_count; i++)
    {
        // delete terminated child process from cli_list
        if (cli_list[i].pid == waitpid(cli_list[i].pid, NULL, WNOHANG))
        {
            printf("Client[%d]'s Release\n", cli_list[i].pid);
            if (i != cli_count - 1)
                cli_list[i] = cli_list[cli_count - 1];
            cli_count--;
            break;
        }
    }
}

// signal alarm handler
void sh_alrm(int signum)
{
    // sort by pid in ascending order
    qsort(cli_list, cli_count, sizeof(struct cli), cmp_pid);

    // print current clients
    printf("Current Number of Client : %d\n", cli_count);
    printf("PID\tPORT\tTIME\n");
    for (int i = 0; i < cli_count; i++)
    {
        cli_list[i].service_time = time(NULL) - cli_list[i].start_time;

        // printf("%d\n", cli_list[i].start_time);
        // printf("%d\n", cli_list[i].service_time);
        // printf("%d\n", time(NULL));
        printf("%d\t%d\t%ld\n", cli_list[i].pid, cli_list[i].port, cli_list[i].service_time);
    }

    // set next alarm
    alarm(10);
}

// signal interrupt handler
void sh_intr(int signum)
{
    printf("SIGINT received. terminating server...\n");
    close(server_fd);

    // close all client fd
    for (int i = 0; i < cli_count; i++)
        close(cli_list[i].cli_fd);

    // terminate all child processes
    for (int i = 0; i < cli_count; i++)
        kill(cli_list[i].pid, SIGTERM);

    // delete cli list
    if (cli_list != NULL)
        free(cli_list);

    exit(0);
}

// compare for sort
int compare(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

// compare for sort cli_list
int cmp_pid(const void *a, const void *b)
{
    struct cli *cli1 = (struct cli *)a;
    struct cli *cli2 = (struct cli *)b;
    return (cli1->pid - cli2->pid);
}

// itos to make string
void int_to_str(int num, char *str)
{
    snprintf(str, 20, "%d", num);
}

// dynamic list management function of cli_list
void resize_list()
{
    int new_capacity = capacity == 0 ? INITCAPA : capacity * 2;
    struct cli *new_list = (struct cli *)realloc(cli_list, new_capacity * sizeof(struct cli));

    if (new_list == NULL)
    {
        printf("Error: mem alloc failed\n");
        exit(-1);
    }

    cli_list = new_list;
    capacity = new_capacity;
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
    else if (strcmp(cmd, "QUIT") == 0)
    {
        //
    }
    else
    {
        strcat(rst_buff, "Error: invalid command\n");
        return -1;
    }

    return 0;
}
