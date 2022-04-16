#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int parseInt(char *src, long *dest)
{
    char *ptr;
    *dest = strtol(src, &ptr, 10);

    if (ptr[0] != '\0')
    {
        fprintf(stderr, "Unexpected character: %s\n", ptr);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int isValidTime(int time)
{
    if (time >= 0 && time <= 1000)
    {
        return EXIT_FAILURE;
    }
    fprintf(stderr, "Invalid time, expected: 0-1000, got: %d.\n", time);
    return EXIT_SUCCESS;
}

int createAtoms(char *name, int count, pid_t pid)
{
    printf("Count: %d\n", count);
    if (pid > 0)
    {
        for (int i = 1; i <= count; i++)
        {
            pid = fork();
            printf("ID: %d\n", pid);
            if (pid == 0) // only child
            {
                printf("Created %s %d\n", name, i);
            }
            else if (pid < 0) // error occured
            {
                return EXIT_FAILURE;
            }
        }
    }
    return EXIT_SUCCESS;
}
int mainProcess = 1;
int main(int argc, char **argv)
{
    if (argc != 5)
    {
        fprintf(stderr, "Invalid count of arguments, expected 4 got: %d\n", argc - 1);
        return 1;
    }

    pid_t pid;
    long no, nh, ti, tb;
    int errOccurred = 0;

    errOccurred += parseInt(argv[1], &no);
    errOccurred += parseInt(argv[2], &nh);
    errOccurred += parseInt(argv[3], &ti);
    errOccurred += parseInt(argv[4], &tb);
    errOccurred += !isValidTime(ti);
    errOccurred += !isValidTime(tb);
    if (errOccurred)
    {
        return EXIT_FAILURE;
    }

    pid = getpid();

    if (mainProcess)
    {
        mainProcess=0;
        errOccurred = createAtoms("NO", no, pid);
        if (errOccurred == EXIT_FAILURE)
        {
            fprintf(stderr, "Error while creating oxygen!");
            exit(EXIT_FAILURE);
        }

        errOccurred = createAtoms("NH", nh, pid);
        if (errOccurred == EXIT_FAILURE)
        {
            fprintf(stderr, "Error while creating hydrogen!");
            exit(EXIT_FAILURE);
        }
    }
}