#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>

sem_t *semaphorOxy = NULL;
sem_t *semaphorHyd = NULL;
FILE *outF;

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
void clear()
{
    sem_close(semaphorOxy);
    sem_close(semaphorHyd);
    sem_unlink("/xmasek19.IOS.Projekt2.O");
    sem_unlink("/xmasek19.IOS.Projekt2.H");
}

int init()
{
    outF = fopen("proj2.out","w");
    if ((semaphorOxy = sem_open("/xmasek19.IOS.Projekt2.O", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {
        printf("Semphore O not created\n");
        return EXIT_FAILURE;
    }
    if ((semaphorHyd = sem_open("/xmasek19.IOS.Projekt2.H", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {
        printf("Semphore H not created\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void mysleep(int max)
{
    int time = rand() % max + 1;
    usleep(time);
    return;
}
void handleOxygen(int id, int ti, int tb)
{
    mysleep(ti);
    printf("A: O %d: going to queue\n", id);
    sem_wait(semaphorOxy);
    exit(0);
}

void handleHydrogen(int id, int ti, int tb)
{
    mysleep(ti);
    printf("A: H %d: going to queue\n", id);
    sem_wait(semaphorHyd);
    exit(0);
}

int main(int argc, char **argv)
{
    clear();
    if (argc != 5)
    {
        fprintf(stderr, "Invalid count of arguments, expected 4 got: %d\n", argc - 1);
        return 1;
    }

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

    pid_t pidParent, pid;
    pidParent = getpid();
    if (init())
    {
        exit(EXIT_FAILURE);
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
            handleOxygen(i, ti, tb);
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
            handleHydrogen(i, ti, tb);
            break;
        }
        else if (pid < 0) // error occured
        {
            fprintf(stderr, "Error while creating hydrogen!");
            clear();
            return EXIT_FAILURE;
        }
    }

    sleep(1);
    int num;
    sem_getvalue(semaphorOxy, &num);
    printf("No O start: %d\n", num);

    sem_post(semaphorOxy);
    // sem_post(semaphorHyd);
    // sem_post(semaphorHyd);

    sem_getvalue(semaphorOxy, &num);
    printf("No O end: %d\n", num);
    sleep(1);
    sem_getvalue(semaphorOxy, &num);
    printf("No O end: %d\n", num);


    printf("end");
    clear();
    exit(0);
}