#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

#define map(var) (mmap(NULL, sizeof(*var), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));

sem_t *oxySemaphore = NULL;
sem_t *hydSemaphore = NULL;
sem_t *writeSem = NULL;
sem_t *moleculeDoneSemH = NULL;
sem_t *moleculeDoneSemO = NULL;

struct shared_t
{
    int moleculeId;
    int numOfOxy;
    int numOfHyd;
    int row;
    long NO;
    long NH;
};

struct shared_t *shared = NULL;

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
    munmap(shared, sizeof *shared);
    sem_close(oxySemaphore);
    sem_close(hydSemaphore);
    sem_close(writeSem);
    sem_close(moleculeDoneSemH);
    sem_close(moleculeDoneSemO);
    sem_unlink("/xmasek19.IOS.Projekt2.O");
    sem_unlink("/xmasek19.IOS.Projekt2.H");
    sem_unlink("/xmasek19.IOS.Projekt2.Write");
    sem_unlink("/xmasek19.IOS.Projekt2.moleculeDoneSemH");
    sem_unlink("/xmasek19.IOS.Projekt2.moleculeDoneSemO");
    fclose(stdout);
}

int init()
{
    freopen("proj2.out", "w", stdout);
    setbuf(stdout, NULL);

    if ((oxySemaphore = sem_open("/xmasek19.IOS.Projekt2.O", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        printf("Semaphore O not created\n");
        return EXIT_FAILURE;
    }

    if ((hydSemaphore = sem_open("/xmasek19.IOS.Projekt2.H", O_CREAT | O_EXCL, 0666, 2)) == SEM_FAILED)
    {
        printf("Semaphore H not created\n");
        return EXIT_FAILURE;
    }

    if ((writeSem = sem_open("/xmasek19.IOS.Projekt2.Write", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        printf("Semaphore write not created\n");
        return EXIT_FAILURE;
    }

    if ((moleculeDoneSemH = sem_open("/xmasek19.IOS.Projekt2.moleculeDoneSemH", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {
        printf("Semaphore moleculeDoneSemH not created\n");
        return EXIT_FAILURE;
    }

    if ((moleculeDoneSemO = sem_open("/xmasek19.IOS.Projekt2.moleculeDoneSemO", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {
        printf("Semaphore moleculeDoneSemO not created\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// FIX ME returns same time
void mysleep(int max)
{
    int time = (rand() % max) + 1;
    // fprintf(stderr,"Time: %d!\n",time);
    usleep(time);
    return;
}

// sync print
void syncPrintAtom(char string[], int row, int atomID)
{
    sem_wait(writeSem);
    printf(string, row, atomID);
    sem_post(writeSem);
}

void syncPrintMolecule(char string[], int row, int atomID, int moleculeID)
{
    sem_wait(writeSem);
    printf(string, row, atomID, moleculeID);
    sem_post(writeSem);
}

void handleOxygen(int id, int TI, int TB)
{
    // created
    syncPrintAtom("%d: O %d: started\n", shared->row += 1, id);

    // wait and join queue
    mysleep(TI);
    syncPrintAtom("%d: O %d: going to queue\n", shared->row += 1, id);

    // wait for creating molecule
    sem_wait(oxySemaphore);
    if (shared->NH - shared->numOfHyd < 2)
    {
        syncPrintAtom("%d: O %d: not enough H\n", shared->row += 1, id);
        sem_post(oxySemaphore);
        exit(0);
    }

    syncPrintMolecule("%d: O %d: creating molecule %d \n", shared->row += 1, id, shared->moleculeId);

    mysleep(TB);

    // inform H (molecule created)
    sem_post(moleculeDoneSemO);
    sem_post(moleculeDoneSemO);

    syncPrintMolecule("%d: O %d: molecule %d created\n", shared->row += 1, id, shared->moleculeId);

    // wait for H created
    sem_wait(moleculeDoneSemH);
    sem_wait(moleculeDoneSemH);

    shared->moleculeId++;
    shared->numOfOxy++;
    sem_post(hydSemaphore);
    sem_post(hydSemaphore);
    sem_post(oxySemaphore);
    exit(0);
}

void handleHydrogen(int id, int TI)
{
    syncPrintAtom("%d: H %d: started\n", shared->row += 1, id);

    mysleep(TI);
    syncPrintAtom("%d: H %d: going to queue\n", shared->row += 1, id);

    sem_wait(hydSemaphore);
    if (shared->NH - shared->numOfHyd < 2 || shared->NO == shared->numOfOxy)
    {
        syncPrintAtom("%d: H %d: not enough O or H\n", shared->row += 1, id);
        sem_post(hydSemaphore);
        exit(0);
    }

    syncPrintMolecule("%d: H %d: creating molecule %d \n", shared->row += 1, id, shared->moleculeId);

    sem_wait(moleculeDoneSemO);
    syncPrintMolecule("%d: H %d: molecule %d created\n", shared->row += 1, id, shared->moleculeId);

    shared->numOfHyd++;
    sem_post(moleculeDoneSemH);

    exit(0);
}

int main(int argc, char **argv)
{
    clear();
    if (argc != 5)
    {
        fprintf(stderr, "Invalid count of arguments, expected 4 got: %d\n", argc - 1);
        clear();
        return 1;
    }

    long NO, NH, TI, TB;
    int errOccurred = 0;

    errOccurred += parseInt(argv[1], &NO);
    errOccurred += parseInt(argv[2], &NH);
    errOccurred += parseInt(argv[3], &TI);
    errOccurred += parseInt(argv[4], &TB);
    errOccurred += !isValidTime(TI);
    errOccurred += !isValidTime(TB);
    if (errOccurred)
    {
        return EXIT_FAILURE;
        clear();
    }

    if (init())
    {
        exit(EXIT_FAILURE);
        clear();
    }

    pid_t pid;
    shared = map(shared);
    shared->moleculeId = 1;
    shared->row = 0;
    shared->NO = NO;
    shared->NH = NH;

    for (int i = 1; i <= NH; i++)
    {
        pid = fork();
        if (pid == 0) // only child
        {
            handleHydrogen(i, TI);
        }
        else if (pid < 0) // error occured
        {
            fprintf(stderr, "Error while creating hydrogen!");
            clear();
            return EXIT_FAILURE;
        }
    }

    for (int i = 1; i <= NO; i++)
    {
        pid = fork();
        if (pid == 0) // only child
        {
            handleOxygen(i, TI, TB);
        }
        else if (pid < 0) // error occured
        {
            fprintf(stderr, "Error while creating oxygen!");
            clear();
            return EXIT_FAILURE;
        }
    }

    // wait for all kids to die
    for (int i = 0; i < NO + NH; i++)
    {
        wait(NULL);
    }
    clear();
    exit(0);
}