#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

#define map(var) (mmap(NULL, sizeof(*var), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));

struct shared_t
{
    int moleculeID;
    int NoOUsed; // number of oxygens used for creating molecules
    int NoHUsed; // number of hydrogens used for creating molecules
    int row;
    long NO;
    long NH;
};
struct shared_t *shared = NULL;

sem_t *oxySemaphore = NULL;
sem_t *hydSemaphore = NULL;
sem_t *writeSem = NULL;
sem_t *moleculeDoneSemH = NULL;
sem_t *moleculeDoneSemO = NULL;

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

void mysleep(int max)
{
    if (max == 0)
    {
        return;
    }
    int time = (rand() % max) + 1;
    time *= 1000;
    usleep(time);
    return;
}

void syncPrintAtom(char string[], struct shared_t *shared, int atomID)
{
    sem_wait(writeSem);
    printf(string, shared->row, atomID);
    shared->row++;
    sem_post(writeSem);
}

void syncPrintMolecule(char string[], struct shared_t *shared, int atomID)
{
    sem_wait(writeSem);
    printf(string, shared->row, atomID, shared->moleculeID);
    shared->row++;
    sem_post(writeSem);
}

void handleOxygen(int id, int TI, int TB)
{
    syncPrintAtom("%d: O %d: started\n", shared, id);
    mysleep(TI);
    syncPrintAtom("%d: O %d: going to queue\n", shared, id);

    // wait while another molecule is being created
    sem_wait(oxySemaphore);
    if (shared->NH - shared->NoHUsed < 2)
    {
        syncPrintAtom("%d: O %d: not enough H\n", shared, id);
        sem_post(oxySemaphore);
        exit(0);
    }

    // create molecule
    syncPrintMolecule("%d: O %d: creating molecule %d \n", shared, id);
    mysleep(TB);

    // inform two hydrogens (molecule created)
    sem_post(moleculeDoneSemO);
    sem_post(moleculeDoneSemO);

    syncPrintMolecule("%d: O %d: molecule %d created\n", shared, id);

    // wait for hydrogens to ack that molecule is created
    sem_wait(moleculeDoneSemH);
    sem_wait(moleculeDoneSemH);

    shared->moleculeID++;
    shared->NoOUsed++;

    // let 1 oxygen and 2 hydrogens to create another molecule
    sem_post(hydSemaphore);
    sem_post(hydSemaphore);
    sem_post(oxySemaphore);
    exit(0);
}

void handleHydrogen(int id, int TI)
{
    syncPrintAtom("%d: H %d: started\n", shared, id);
    mysleep(TI);
    syncPrintAtom("%d: H %d: going to queue\n", shared, id);

    // wait while another molecule is being created
    sem_wait(hydSemaphore);
    if (shared->NH - shared->NoHUsed < 2 || shared->NO == shared->NoOUsed)
    {
        syncPrintAtom("%d: H %d: not enough O or H\n", shared, id);
        sem_post(hydSemaphore);
        exit(0);
    }

    // create molecule
    syncPrintMolecule("%d: H %d: creating molecule %d \n", shared, id);

    sem_wait(moleculeDoneSemO);
    syncPrintMolecule("%d: H %d: molecule %d created\n", shared, id);

    shared->NoHUsed++;
    // ack for oxygen that molecule is created
    sem_post(moleculeDoneSemH);
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc != 5)
    {
        fprintf(stderr, "Invalid count of arguments, expected 4 got: %d\n", argc - 1);
        clear();
        return 1;
    }

    pid_t pid;
    long NO, NH, TI, TB;
    int errOccurred = 0;
    shared = map(shared);

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

    shared->moleculeID = 1;
    shared->row = 1;
    shared->NO = NO;
    shared->NH = NH;

    // hydrogen generator
    for (int id = 1; id <= NH; id++)
    {
        pid = fork();
        if (pid == 0) // child only
        {
            handleHydrogen(id, TI);
        }
        else if (pid < 0) // error occured
        {
            fprintf(stderr, "Error while creating hydrogen!");
            clear();
            return EXIT_FAILURE;
        }
    }

    // oxygen generator
    for (int id = 1; id <= NO; id++)
    {
        pid = fork();
        if (pid == 0) // child only
        {
            handleOxygen(id, TI, TB);
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