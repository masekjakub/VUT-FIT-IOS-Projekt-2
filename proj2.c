#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>

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

void mysleep(int max){
    int time = rand()%max+1;
    usleep(time);
    return;
}
int handleOxygen(int id, int ti)
{
    mysleep(ti);
    printf("A: O %d: going to queue\n", id);
    return 0;
}

int handleHydrogen(int id, int ti)
{
    mysleep(ti);
    printf("A: H %d: going to queue\n", id);
    return 0;
}

sem_t *semaphorOxy;

int main(int argc, char **argv)
{
    if (argc != 5)
    {
        fprintf(stderr, "Invalid count of arguments, expected 4 got: %d\n", argc - 1);
        return 1;
    }

    pid_t pidParent, pid;
    long no, nh, ti, tb;
    int errOccurred = 0;
    pidParent = getpid();

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

    for (int i = 1; i <= no; i++)
    {
        pid = getpid();

        if (pid != pidParent)
        {
            break;
        }

        pid = fork();
        if (pid == 0) // only child
        {
            printf("A: O %d: started\n", i);
            handleOxygen(i, ti);
            break;
        }
        else if (pid < 0) // error occured
        {
            fprintf(stderr, "Error while creating oxygen!");
            return EXIT_FAILURE;
        }
    }

    for (int i = 1; i <= nh; i++)
    {
        pid = getpid();

        if (pid != pidParent)
        {
            break;
        }

        pid = fork();
        if (pid == 0) // only child
        {
            printf("A: H %d: started\n", i);
            handleHydrogen(i, ti);
            break;
        }
        else if (pid < 0) // error occured
        {
            fprintf(stderr, "Error while creating hydrogen!");
            return EXIT_FAILURE;
        }
    }
}