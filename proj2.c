#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

sem_t *semaphorOxy = NULL;
sem_t *semaphorHyd = NULL;
FILE *outF;
int *row = NULL;

void createSharedVar(int *var)
{
    
}

void removeSharedVar(int *var){
    
}
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
    row++;
    printf("%d: O %d: going to queue\n", row, id);
    sem_wait(semaphorOxy);
    exit(0);
}

void handleHydrogen(int id, int ti, int tb)
{
    mysleep(ti);
    row++;
    printf("%d: H %d: going to queue\n", row, id);
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
    mmap(NULL, sizeof *row, PROT_READ | PROT_WRITE, MAP_SHARED, -1, 0);
    row = 0;
    
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
            row++;
            printf("%d: O %d: started\n", row, i);
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
            row++;
            printf("%d: H %d: started\n", row, i);
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
    printf("Row: %d\n", row);

    munmap(row, sizeof *row);
    printf("end");
    clear();
    removeSharedVar(row);
    exit(0);
}