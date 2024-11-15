#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>

// //////////////////////////////////////////////////////////////////////////////////////
// File Name : kw2020202090_ls.c
// Date : 2024/04/05
// OS : Ubuntu 20.04.6 64-bit
//
// Arthor : Min Seok Choi
// Student ID : 2020202090
//
// Title: System Programming Assignment 1-2 (FTP server)
// Description: The purpose of Assignment 1-2 is to implement ls command
//              by using dirent, DIR structure.
// //////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    DIR *dp;
    struct dirent *entry = 0;

    // handling exceptions if there are more than one argument
    if (argc > 2)
    {
        printf("only one directory path can be processed\n");
        return 0;
    }

    // open current directory or path's directory
    if (argv[1] == 0)
        dp = opendir("./");
    else
        dp = opendir(argv[1]);

    // error handling
    if (dp == NULL)
    {
        if (access(argv[1], F_OK) == -1)
        {
            printf("%s: cannot access \"%s\" : No such directory\n", argv[0], argv[1]);
            return 0;
        }

        if (access(argv[1], R_OK) == -1)
        {
            printf("%s: cannot access \"%s\" : Access denied\n", argv[0], argv[1]);
            return 0;
        }
    }

    // read directory entry and print it ont by one
    while ((entry = readdir(dp)) != 0)
        printf("%s\n", entry->d_name);

    // close directory + error handling
    if (closedir(dp))
        printf("closed error !!!\n");

    return 0;
}