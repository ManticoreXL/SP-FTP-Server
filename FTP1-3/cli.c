#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFSIZE 256

// //////////////////////////////////////////////////////////////////////////////////////
// File Name : cli.c
// Date : 2024/04/16
// OS : Ubuntu 20.04.6 64-bit
//
// Arthor : Min Seok Choi
// Student ID : 2020202090
//
// Title: System Programming Assignment 1-3 (FTP server)
// Description: The purpose of Assignment 1-3 is to implement ftp file system (client)
// //////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    char ftpcmd[BUFSIZE] = "";

    if (strcmp(argv[1], "ls") == 0)
    {
        // name list
        strcat(ftpcmd, "NLST");
        for (int i = 2; i < argc; i++)
        {
            strcat(ftpcmd, " ");
            strcat(ftpcmd, argv[i]);
        }
    }
    else if (strcmp(argv[1], "dir") == 0)
    {
        // directory entry list (ls -al)
        strcat(ftpcmd, "LIST");
        for (int i = 2; i < argc; i++)
        {
            strcat(ftpcmd, " ");
            strcat(ftpcmd, argv[i]);
        }
    }
    else if (strcmp(argv[1], "pwd") == 0)
    {
        // print working directory
        strcat(ftpcmd, "PWD");
        for (int i = 2; i < argc; i++)
        {
            strcat(ftpcmd, " ");
            strcat(ftpcmd, argv[i]);
        }
    }
    else if (strcmp(argv[1], "cd") == 0)
    {
        // change working directory

        if (strcmp(argv[2], "..") == 0)
            strcat(ftpcmd, "CDUP");
        else
            strcat(ftpcmd, "CWD");
        for (int i = 2; i < argc; i++)
        {
            strcat(ftpcmd, " ");
            strcat(ftpcmd, argv[i]);
        }
    }
    else if (strcmp(argv[1], "mkdir") == 0)
    {
        // make directory
        strcat(ftpcmd, "MKD");
        for (int i = 2; i < argc; i++)
        {
            strcat(ftpcmd, " ");
            strcat(ftpcmd, argv[i]);
        }
    }
    else if (strcmp(argv[1], "delete") == 0)
    {
        // delete file
        strcat(ftpcmd, "DELE");
        for (int i = 2; i < argc; i++)
        {
            strcat(ftpcmd, " ");
            strcat(ftpcmd, argv[i]);
        }
    }
    else if (strcmp(argv[1], "rmdir") == 0)
    {
        // remove directory
        strcat(ftpcmd, "RMD");
        for (int i = 2; i < argc; i++)
        {
            strcat(ftpcmd, " ");
            strcat(ftpcmd, argv[i]);
        }
    }
    else if (strcmp(argv[1], "rename") == 0)
    {
        // rename file
        strcat(ftpcmd, "RNFR ");
        if (argc >= 3)
            strcat(ftpcmd, argv[2]);
        strcat(ftpcmd, " RNTO ");
        if (argc >= 4)
            strcat(ftpcmd, argv[3]);
    }
    else if (strcmp(argv[1], "quit") == 0)
    {
        // quit
        strcat(ftpcmd, "QUIT");
        for (int i = 2; i < argc; i++)
        {
            strcat(ftpcmd, " ");
            strcat(ftpcmd, argv[i]);
        }
    }

    printf("%s\n", ftpcmd);

    // wrtie to stdout
    write(1, ftpcmd, BUFSIZE - 1);

    return 0;
}