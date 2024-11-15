#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define BUFSIZE 256

// //////////////////////////////////////////////////////////////////////////////////////
// File Name : srv.c
// Date : 2024/04/16
// OS : Ubuntu 20.04.6 64-bit
//
// Arthor : Min Seok Choi
// Student ID : 2020202090
//
// Title: System Programming Assignment 1-3 (FTP server)
// Description: The purpose of Assignment 1-3 is to implement ftp file system (server)
// //////////////////////////////////////////////////////////////////////////////////////

int compare(const void *a, const void *b);
void list_dir(DIR *dp, int aflag, int lflag);
void int_to_str(int num, char *str);

int main()
{
    char ftpcmd[BUFSIZE] = "";
    char cmd[16] = "";
    char argv1[64] = "";
    char argv2[64] = "";
    char argv3[64] = "";

    // read from stdin
    read(0, ftpcmd, BUFSIZE - 1);

    // split ftpcmd string
    sscanf(ftpcmd, "%s %s %s %s", cmd, argv1, argv2, argv3);

    // debug section
    // printf("%s\n", ftpcmd);
    // printf("command: %s\n", cmd);
    // printf("Argument 1: %s\n", argv1);
    // printf("Argument 2: %s\n", argv2);
    // printf("Argument 3: %s\n", argv3);
    // printf("file exist: %d", access(argv2, F_OK));
    // printf("read permission: %d", access(argv2, R_OK));

    // execute command
    if (strcmp(cmd, "NLST") == 0) // name list
    {
        DIR *dp;
        int aflag = 0;
        int lflag = 0;

        // open curr dir or path's dir
        if (strcmp(argv1, "") == 0)
        {
            // ls with no argument
            dp = opendir("./");
        }
        else if (strcmp(argv2, "") == 0)
        {
            // ls with 1 argument
            if (argv1[0] == '-')
            {
                // ls + option
                dp = opendir("./");

                // enable option flag
                if (strcmp(argv1, "-al") == 0 || strcmp(argv1, "-la") == 0)
                {
                    aflag = 1;
                    lflag = 1;
                }
                else if (strcmp(argv1, "-a") == 0)
                    aflag = 1;
                else if (strcmp(argv1, "-l") == 0)
                    lflag = 1;
                else
                {
                    write(1, "Error: invalid option\n", strlen("Error: invalid option\n"));
                    return 0;
                }
            }
            else if (argv1[0] == '/')
            {
                // ls + path
                dp = opendir(argv1);
            }
            else
            {
                // ls + file
                write(1, cmd, strlen(cmd));
                write(1, "\n", 1);

                // error handling
                if (access(argv1, F_OK) == -1)
                {
                    write(1, "Error: No such file or directory", strlen("Error: No such file or directory"));
                    write(1, "\n", 1);
                    return 0;
                }
                if (access(argv1, R_OK) == -1)
                {
                    write(1, "Error: cannot access", strlen("Error: cannot access"));
                    write(1, "\n", 1);
                    return 0;
                }

                // print fileinfo
                write(1, argv1, strlen(argv1));
                write(1, "\n", 1);
                return 0;
            }
        }
        else
        {
            // ls with 2 arguments

            // enable option
            if (strcmp(argv1, "-al") == 0)
            {
                aflag = 1;
                lflag = 1;
            }
            else if (strcmp(argv1, "-a") == 0)
                aflag = 1;
            else if (strcmp(argv1, "-l") == 0)
                lflag = 1;
            else
            {
                write(1, "Error: invalid option\n", strlen("Error: invalid option\n"));
                return 0;
            }

            if (argv2[0] == '/')
            {
                // ls + option + path
                dp = opendir(argv2);
            }
            else if (strcmp(argv2, "") != 0)
            {
                // ls + option + file
                struct stat statbuf;
                struct passwd *pwd;
                struct group *grp;
                struct tm *timeinfo;

                write(1, cmd, strlen(cmd));
                if (aflag && lflag)
                    write(1, " -al", strlen(" -al"));
                else if (aflag)
                    write(1, " -a", strlen(" -a"));
                else if (lflag)
                    write(1, " -l", strlen(" -l"));
                write(1, "\n", 1);

                // error handling
                if (access(argv2, F_OK) == -1)
                {
                    write(1, "Error: No such file or directory", strlen("Error: No such file or directory"));
                    write(1, "\n", 1);
                    return 0;
                }
                if (access(argv2, R_OK) == -1)
                {
                    write(1, "Error: cannot access", strlen("Error: cannot access"));
                    write(1, "\n", 1);
                    return 0;
                }

                // get status
                stat(argv2, &statbuf);

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
                    return 0;
                }

                // get group name
                if ((grp = getgrgid(statbuf.st_gid)) == NULL)
                {
                    write(1, "Error: failed to get group name\n", strlen("Error: failed to get group name\n"));
                    return 0;
                }

                // get size string
                char size_str[20];
                int_to_str((int)statbuf.st_size, size_str);

                // get time string
                timeinfo = localtime(&statbuf.st_mtime);
                char time_str[32];
                strftime(time_str, 32, "%b %d %H:%M", timeinfo);

                // print fileinfo
                write(1, perms, strlen(perms));
                write(1, " ", 1);
                write(1, ln_str, strlen(ln_str));
                write(1, " ", 1);
                write(1, pwd->pw_name, strlen(pwd->pw_name));
                write(1, " ", 1);
                write(1, grp->gr_name, strlen(grp->gr_name));
                write(1, " ", 1);
                write(1, size_str, strlen(size_str));
                write(1, " ", 1);
                write(1, time_str, strlen(time_str));
                write(1, " ", 1);
                write(1, argv2, strlen(argv2));
                write(1, (S_ISDIR(statbuf.st_mode)) ? "/\n" : "\n", 2);
                return 0;
            }
        }

        write(1, cmd, strlen(cmd));
        if (aflag && lflag)
            write(1, " -al", strlen(" -al"));
        else if (aflag)
            write(1, " -a", strlen(" -a"));
        else if (lflag)
            write(1, " -l", strlen(" -l"));
        write(1, "\n", 1);

        // namelist
        list_dir(dp, aflag, lflag);

        if (closedir(dp))
            write(1, "closed error!!!\n", strlen("closed error!!!\n"));
    }
    else if (strcmp(cmd, "LIST") == 0) // directory entry list (ls -al)
    {
        DIR *dp;
        struct dirent *entry = 0;

        // open curr dir or path's dir
        if (strcmp(argv1, "") == 0)
            dp = opendir("./");
        else
            dp = opendir(argv1);

        write(1, cmd, strlen(cmd));
        write(1, "\n", 1);

        // error handling
        if (argv1[0] == '-')
        {
            write(1, "Error: invalid option\n", strlen("Error: invalid option\n"));
            return 0;
        }
        if (strcmp(argv1, "") != 0 && access(argv1, F_OK) == -1)
        {
            write(1, "Error: cannot access \"", strlen("Error: cannot access \""));
            write(1, argv1, strlen(argv1));
            write(1, "\" : No such file or directory\n", strlen("\" : No such file or directory\n"));
            return 0;
        }
        if (strcmp(argv1, "") != 0 && access(argv1, R_OK) == -1)
        {
            write(1, "Error: cannot access \"", strlen("Error: cannot access \""));
            write(1, argv1, strlen(argv1));
            write(1, "\" : Access denied\n", strlen("\" : Access denied\n"));
            return 0;
        }

        // namelist -al
        list_dir(dp, 1, 1);

        if (closedir(dp))
            write(1, "closed error!!!\n", strlen("closed error!!!\n"));
    }
    else if (strcmp(cmd, "PWD") == 0) // print working directory
    {
        char currdir[64] = "";
        getcwd(currdir, 63);

        // error handling
        if (strcmp(argv1, "") != 0)
        {
            if (argv1[0] == '-')
                write(1, "Error: invalid option\n", strlen("Error: invalid option\n"));
            else
                write(1, "Error: argument is not required\n", strlen("Error: argument is not required\n"));

            return 0;
        }

        // print current directory
        write(1, "\"", 1);
        write(1, currdir, strlen(currdir));
        write(1, "\" is current directory\n", strlen("\" is current directory\n"));
    }
    else if (strcmp(cmd, "CWD") == 0) // change working directory
    {
        // error handling
        if (strcmp(argv1, "") == 0)
        {
            write(1, "Error: argument is required\n", strlen("Error: argument is required\n"));
            return 0;
        }
        if (argv1[0] == '-')
        {
            write(1, "Error: invalid option\n", strlen("Error: invalid option\n"));
            return 0;
        }
        if (strcmp(argv2, "") != 0)
        {
            write(1, "Error: only one argument is required\n", strlen("Error: only one argument is required\n"));
            return 0;
        }

        // change directory
        if (chdir(argv1) != 0)
        {
            write(1, "Error: directory not found\n", strlen("Error: directory not found\n"));
            return 0;
        }

        write(1, cmd, strlen(cmd));
        write(1, " ", 1);
        write(1, argv1, strlen(argv1));
        write(1, "\n", 1);

        // print current directory
        char currdir[64] = "";
        getcwd(currdir, 63);
        write(1, "\"", 1);
        write(1, currdir, strlen(currdir));
        write(1, "\" is current directory\n", strlen("\" is current directory\n"));
    }
    else if (strcmp(cmd, "CDUP") == 0) // change to parent directory
    {
        // error handling
        if (argv1[0] == '-')
        {
            write(1, "Error: invalid option\n", strlen("Error: invalid option\n"));
            return 0;
        }

        if (chdir("..") != 0)
        {
            write(1, "Error: directory not found\n", strlen("Error: directory not found\n"));
            return 0;
        }

        write(1, cmd, strlen(cmd));
        write(1, "\n", 1);

        // print current directory
        char currdir[64] = "";
        getcwd(currdir, 63);
        write(1, "\"", 1);
        write(1, currdir, strlen(currdir));
        write(1, "\" is current directory\n", strlen("\" is current directory\n"));
    }
    else if (strcmp(cmd, "MKD") == 0) // make directory
    {
        // try to make directory argv1
        if (strcmp(argv1, "") == 0)
        {
            write(1, "Error: argument is required\n", strlen("Error: argument is required\n"));
            return 0;
        }
        else if (argv1[0] == '-')
        {
            write(1, "Error: invalid option\n", strlen("Error: invalid option\n"));
            return 0;
        }
        else if (access(argv1, F_OK) != -1)
        {
            write(1, "Error: cannot create directory \'", strlen("Error: cannot create directory \'"));
            write(1, argv1, strlen(argv1));
            write(1, "\': File exists\n", strlen("\': File exists\n"));
        }
        else
        {
            // make directory - argv1
            if (mkdir(argv1, 0755) == 0)
            {
                write(1, "MKD ", strlen("MKD "));
                write(1, argv1, strlen(argv1));
                write(1, "\n", 1);
            }
            else
                write(1, "Error: failed to make directory\n", strlen("Error: failed to make directory\n"));
        }

        // try to make directory argv2
        if (argv2[0] == '-')
        {
            write(1, "Error: invalid option\n", strlen("Error: invalid option\n"));
            return 0;
        }
        else if (access(argv2, F_OK) != -1)
        {
            write(1, "Error: cannot create directory \'", strlen("Error: cannot create directory \'"));
            write(1, argv2, strlen(argv2));
            write(1, "\': File exists\n", strlen("\': File exists\n"));
        }
        else
        {
            // make directory - argv2
            if (strcmp(argv2, "") != 0)
                if (mkdir(argv2, 0755) == 0)
                {
                    write(1, "MKD ", strlen("MKD "));
                    write(1, argv2, strlen(argv2));
                    write(1, "\n", 1);
                }
                else
                    write(1, "Error: failed to make directory\n", strlen("Error: failed to make directory\n"));
        }

        // try to make directory argv3
        if (argv3[0] == '-')
        {
            write(1, "Error: invalid option\n", strlen("Error: invalid option\n"));
            return 0;
        }
        else if (access(argv3, F_OK) != -1)
        {
            write(1, "Error: cannot create directory \'", strlen("Error: cannot create directory \'"));
            write(1, argv3, strlen(argv3));
            write(1, "\': File exists\n", strlen("\': File exists\n"));
        }
        else
        {
            // make directory - argv3
            if (strcmp(argv3, "") != 0)
                if (mkdir(argv3, 0755) == 0)
                {
                    write(1, "MKD ", strlen("MKD "));
                    write(1, argv3, strlen(argv3));
                    write(1, "\n", 1);
                }
                else
                    write(1, "Error: failed to make directory\n", strlen("Error: failed to make directory\n"));
        }
    }
    else if (strcmp(cmd, "DELE") == 0) // delete file
    {
        // try to delete argv1
        if (strcmp(argv1, "") == 0)
        {
            write(1, "Error: argument is required\n", strlen("Error: argument is required\n"));
            return 0;
        }
        else if (argv1[0] == '-')
        {
            write(1, "Error: invalid option\n", strlen("Error: invalid option\n"));
            return 0;
        }
        else if (access(argv1, F_OK) == -1)
        {
            write(1, "Error: failed to delete \'", strlen("Error: failed to delete \'"));
            write(1, argv1, strlen(argv1));
            write(1, "\'\n", strlen("\'\n"));
        }
        else
        {
            // delete directory - argv1
            if (unlink(argv1) == 0)
            {
                write(1, "DELE ", strlen("DELE "));
                write(1, argv1, strlen(argv1));
                write(1, "\n", 1);
            }
            else
                write(1, "Error: failed to delete directory\n", strlen("Error: failed to delete directory\n"));
        }

        // try to delete argv2
        if (strcmp(argv2, "") != 0)
        {
            if (argv2[0] == '-')
            {
                write(1, "Error: invalid option\n", strlen("Error: invalid option\n"));
                return 0;
            }
            if (access(argv2, F_OK) == -1)
            {
                write(1, "Error: failed to delete \'", strlen("Error: failed to delete \'"));
                write(1, argv2, strlen(argv2));
                write(1, "\'\n", strlen("\'\n"));
            }
            else
            {
                // delete directory - argv2
                if (strcmp(argv2, "") != 0)
                    if (unlink(argv2) == 0)
                    {
                        write(1, "DELE ", strlen("DELE "));
                        write(1, argv2, strlen(argv2));
                        write(1, "\n", 1);
                    }
                    else
                        write(1, "Error: failed to delete directory\n", strlen("Error: failed to delete directory\n"));
            }
        }

        // try to delete argv3
        if (strcmp(argv3, "") != 0)
        {
            if (argv3[0] == '-')
            {
                write(1, "Error: invalid option\n", strlen("Error: invalid option\n"));
                return 0;
            }
            if (access(argv3, F_OK) == -1)
            {
                write(1, "Error: failed to delete \'", strlen("Error: failed to delete \'"));
                write(1, argv3, strlen(argv3));
                write(1, "\'\n", strlen("\'\n"));
            }
            else
            {
                // delete directory - argv3
                if (strcmp(argv3, "") != 0)
                    if (unlink(argv3) == 0)
                    {
                        write(1, "DELE ", strlen("DELE "));
                        write(1, argv3, strlen(argv3));
                        write(1, "\n", 1);
                    }
                    else
                        write(1, "Error: failed to delete directory\n", strlen("Error: failed to delete directory\n"));
            }
        }
    }
    else if (strcmp(cmd, "RMD") == 0) // remove empty directory
    {
        // try to remove directory argv1
        if (strcmp(argv1, "") == 0)
        {
            write(1, "Error: argument is required\n", strlen("Error: argument is required\n"));
            return 0;
        }
        else if (argv1[0] == '-')
        {
            write(1, "Error: invalid option\n", strlen("Error: invalid option\n"));
            return 0;
        }
        else if (access(argv1, F_OK) == -1)
        {
            write(1, "Error: failed to remove \'", strlen("Error: failed to remove \'"));
            write(1, argv1, strlen(argv1));
            write(1, "\'\n", strlen("\'\n"));
        }
        else
        {
            // delete directory - argv1
            if (rmdir(argv1) == 0)
            {
                write(1, "RMD ", strlen("RMD "));
                write(1, argv1, strlen(argv1));
                write(1, "\n", 1);
            }
            else
                write(1, "Error: failed to remove directory\n", strlen("Error: failed to remove directory\n"));
        }

        // try to remove directory argv2
        if (strcmp(argv2, "") != 0)
        {
            if (argv2[0] == '-')
            {
                write(1, "Error: invalid option\n", strlen("Error: invalid option\n"));
                return 0;
            }
            else if (access(argv2, F_OK) == -1)
            {
                write(1, "Error: failed to remove \'", strlen("Error: failed to remove \'"));
                write(1, argv2, strlen(argv2));
                write(1, "\'\n", strlen("\'\n"));
            }
            else
            {
                // delete directory - argv2
                if (strcmp(argv2, "") != 0)
                    if (rmdir(argv2) == 0)
                    {
                        write(1, "RMD ", strlen("RMD "));
                        write(1, argv2, strlen(argv2));
                        write(1, "\n", 1);
                    }
                    else
                        write(1, "Error: failed to remove directory\n", strlen("Error: failed to remove directory\n"));
            }
        }

        // try to remove directory argv3
        if (strcmp(argv3, "") != 0)
        {
            if (argv3[0] == '-')
            {
                write(1, "Error: invalid option\n", strlen("Error: invalid option\n"));
                return 0;
            }
            else if (access(argv3, F_OK) == -1)
            {
                write(1, "Error: failed to remove \'", strlen("Error: failed to remove \'"));
                write(1, argv3, strlen(argv3));
                write(1, "\'\n", strlen("\'\n"));
            }
            else
            {
                // delete directory - argv3
                if (strcmp(argv3, "") != 0)
                    if (rmdir(argv3) == 0)
                    {
                        write(1, "RMD ", strlen("RMD "));
                        write(1, argv3, strlen(argv3));
                        write(1, "\n", 1);
                    }
                    else
                        write(1, "Error: failed to remove directory\n", strlen("Error: failed to remove directory\n"));
            }
        }
    }
    else if (strcmp(cmd, "RNFR") == 0) // rename file/directory
    {
        // error handling
        if (strcmp(argv1, "RNTO") == 0 || strcmp(argv3, "") == 0)
        {
            write(1, "Error: two arguments are required\n", strlen("Error: two arguments are required\n"));
            return 0;
        }
        if (access(argv1, F_OK) == -1)
        {
            write(1, "Error: file to change doesn't exist\n", strlen("Error: file to change doesn't exist\n"));
            return 0;
        }
        if (access(argv3, F_OK) != -1)
        {
            write(1, "Error: name to change already exists\n", strlen("Error: name to change already exists\n"));
            return 0;
        }

        // rename argv1 to argv3
        if (rename(argv1, argv3) == 0)
        {
            write(1, "RNFR ", strlen("RNFR "));
            write(1, argv1, strlen(argv1));
            write(1, "\n", 1);
            write(1, "RNTO ", strlen(" RNTO "));
            write(1, argv3, strlen(argv3));
            write(1, "\n", 1);
        }
        else
            write(1, "Error: failed to change filename\n", strlen("Error: failed to change filename\n"));
    }
    else if (strcmp(cmd, "QUIT") == 0) // quit
    {
        // error handling
        if (strcmp(argv1, "") != 0)
        {
            if (argv1[0] == '-')
                write(1, "Error: invalid option\n", strlen("Error: invalid option\n"));
            else
                write(1, "Error: argument is not required\n", strlen("Error: argument is not required\n"));

            return 0;
        }
        // terminate program
        write(1, "QUIT success\n", strlen("QUIT success\n"));
        exit(0);
    }
    else // wrong command
    {
        write(1, "Error: invalid ftp command\n", strlen("Error: invalid ftp command\n"));
    }
    return 0;
}

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

// print directory entry function
void list_dir(DIR *dp, int aflag, int lflag)
{
    struct dirent *entry = 0;
    struct stat statbuf;
    struct passwd *pwd;
    struct group *grp;
    struct tm *timeinfo;
    char **namelist = NULL;
    int count = 0;

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
                continue;
            }

            // get group name
            if ((grp = getgrgid(statbuf.st_gid)) == NULL)
            {
                write(1, "Error: failed to get group name\n", strlen("Error: failed to get group name\n"));
                continue;
            }

            // get size string
            char size_str[20];
            int_to_str((int)statbuf.st_size, size_str);

            // get time string
            timeinfo = localtime(&statbuf.st_mtime);
            char time_str[32];
            strftime(time_str, 32, "%b %d %H:%M", timeinfo);

            // print directory entry -l
            write(1, perms, strlen(perms));
            write(1, " ", 1);
            write(1, ln_str, strlen(ln_str));
            write(1, " ", 1);
            write(1, pwd->pw_name, strlen(pwd->pw_name));
            write(1, " ", 1);
            write(1, grp->gr_name, strlen(grp->gr_name));
            write(1, " ", 1);
            write(1, size_str, strlen(size_str));
            write(1, " ", 1);
            write(1, time_str, strlen(time_str));
            write(1, " ", 1);
            write(1, namelist[i], strlen(namelist[i]));
            write(1, (S_ISDIR(statbuf.st_mode)) ? "/\n" : "\n", 2);
        }
    }
    else // ls + ls -a
    {
        for (int i = 0; i < count; i++)
        {
            // get status
            stat(namelist[i], &statbuf);

            // print directory entry
            write(1, namelist[i], strlen(namelist[i]));
            write(1, (S_ISDIR(statbuf.st_mode)) ? "/" : "", 2);
            if (((i + 1) % 5 == 0))
                write(1, "\n", 1);
            else
                write(1, " ", 1);
        }
        write(1, "\n", 1);
    }

    // delete memory
    for (int i = 0; i < count; i++)
        free(namelist[i]);
    free(namelist);
}