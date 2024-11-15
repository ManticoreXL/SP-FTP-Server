#include <stdio.h>
#include <unistd.h>

// //////////////////////////////////////////////////////////////////////////////////////
// File Name : kw2020202090_opt.c
// Date : 2024/03/31
// OS : Ubuntu 20.04.6 64-bit
//
// Arthor : Min Seok Choi
// Student ID : 2020202090
//
// Title: System Programming Assignment 1-1 (FTP server)
// Description: The purpose of Assignment 1-1 is to parse parameters by using getopt().
// //////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
    int aflag = 0;
    int bflag = 0;
    char *cvalue = NULL;
    int c = 0;

    // call getopt() until getopt() returns -1.
    while ((c = getopt(argc, argv, "abdc:")) != -1)
    {
        // printf("optarg = %s\t optind = %d\t opterr = %d\t optopt = %c\n", optarg, optind, opterr, optopt);

        // parse options
        switch (c)
        {
        case 'a':
            aflag++;
            break;
        case 'b':
            bflag++;
            break;
        case 'c':
            cvalue = optarg;
            break;
        case 'd':
            opterr = 0;
            break;
        case '?':
            printf("Unknown option character\n");
            break;
        }
    }

    // print parsed options
    printf("aflag = %d, bflag = %d, cvalue = %s\n", aflag, bflag, cvalue);

    // print non-option arguments
    if (opterr != 0)
        for (int i = optind; i < argc; i++)
            printf("Non-option argument %s\n", argv[optind]);

    return 0;
}
