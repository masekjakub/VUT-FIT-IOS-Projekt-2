#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

#define map(var) (mmap(NULL, sizeof(*var), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));

FILE *file;

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

sem_t *oxyMolecSem = NULL;
sem_t *hydMolecSem = NULL;
sem_t *writeSem = NULL;
sem_t *oxygenSem = NULL;
sem_t *hydrogenSem = NULL;

int parseLong(char *src, long *dest)
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
    fclose(file);
    sem_close(oxyMolecSem);
    sem_close(hydMolecSem);
    sem_close(writeSem);
    sem_close(oxygenSem);
    sem_close(hydrogenSem);
    sem_unlink("/xmasek19.IOS.Projekt2.oxyMolecSem");
    sem_unlink("/xmasek19.IOS.Projekt2.hydMolecSem");
    sem_unlink("/xmasek19.IOS.Projekt2.Write");
    sem_unlink("/xmasek19.IOS.Projekt2.oxygenSem");
    sem_unlink("/xmasek19.IOS.Projekt2.hydrogenSem");
}

int init()
{
    if ((oxyMolecSem = sem_open("/xmasek19.IOS.Projekt2.oxyMolecSem", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        printf("Semaphore oxyMolecSem not created\n");
        return EXIT_FAILURE;
    }

    if ((hydMolecSem = sem_open("/xmasek19.IOS.Projekt2.hydMolecSem", O_CREAT | O_EXCL, 0666, 2)) == SEM_FAILED)
    {
        printf("Semaphore H not created\n");
        return EXIT_FAILURE;
    }

    if ((writeSem = sem_open("/xmasek19.IOS.Projekt2.Write", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        printf("Semaphore write not created\n");
        return EXIT_FAILURE;
    }

    if ((oxygenSem = sem_open("/xmasek19.IOS.Projekt2.oxygenSem", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {
        printf("Semaphore oxygenSem not created\n");
        return EXIT_FAILURE;
    }

    if ((hydrogenSem = sem_open("/xmasek19.IOS.Projekt2.hydrogenSem", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {
        printf("Semaphore hydrogenSem not created\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void mysleep(int max, int row)
{
    if (max == 0)
    {
        return;
    }
    srand(getpid()/row);
    int time = (rand() % max) + 1;
    time *= 1000;
    usleep(time);
    return;
}

void syncPrintAtom(char string[], struct shared_t *shared, int atomID)
{
    sem_wait(writeSem);
    fprintf(file,string, shared->row, atomID);
    fflush(file);
    shared->row++;
    sem_post(writeSem);
}

void syncPrintMolecule(char string[], struct shared_t *shared, int atomID)
{
    sem_wait(writeSem);
    fprintf(file,string, shared->row, atomID, shared->moleculeID);
    fflush(file);
    shared->row++;
    sem_post(writeSem);
}

void handleOxygen(int id, int TI, int TB)
{
    syncPrintAtom("%d: O %d: started\n", shared, id);
    mysleep(TI, shared->row);
    syncPrintAtom("%d: O %d: going to queue\n", shared, id);

    // wait for previous molecule to be created
    sem_wait(oxyMolecSem);

    if (shared->NH - shared->NoHUsed < 2)
    {
        syncPrintAtom("%d: O %d: not enough H\n", shared, id);
        sem_post(oxyMolecSem);
        return;
    }

    // create molecule
    syncPrintMolecule("%d: O %d: creating molecule %d \n", shared, id);
    mysleep(TB, shared->row);

    // wait or all 3 atoms to start creating
    sem_wait(oxygenSem);
    sem_wait(oxygenSem);
    sem_post(hydrogenSem);
    sem_post(hydrogenSem);

    syncPrintMolecule("%d: O %d: molecule %d created\n", shared, id);

    // wait for all atoms to write "molecule x created"
    sem_post(hydrogenSem);
    sem_post(hydrogenSem);
    sem_wait(oxygenSem);
    sem_wait(oxygenSem);

    // no synchronization needed (only 1 process at the same time)
    shared->moleculeID++;
    shared->NoOUsed++;

    // let 1 oxygen and 2 hydrogens to create another molecule
    sem_post(hydMolecSem);
    sem_post(hydMolecSem);
    sem_post(oxyMolecSem);
    return;
}

void handleHydrogen(int id, int TI)
{
    syncPrintAtom("%d: H %d: started\n", shared, id);
    mysleep(TI, shared->row);
    syncPrintAtom("%d: H %d: going to queue\n", shared, id);

    // wait for previous molecule to be created
    sem_wait(hydMolecSem);

    if (shared->NH - shared->NoHUsed < 2 || shared->NO == shared->NoOUsed)
    {
        syncPrintAtom("%d: H %d: not enough O or H\n", shared, id);
        sem_post(hydMolecSem);
        return;
    }

    syncPrintMolecule("%d: H %d: creating molecule %d \n", shared, id);

    // wait or all 3 atoms to start creating
    sem_post(oxygenSem);
    sem_wait(hydrogenSem);
    syncPrintMolecule("%d: H %d: molecule %d created\n", shared, id);

    // synchronized operation with shared variable
    sem_wait(writeSem);
    shared->NoHUsed++;
    sem_post(writeSem);

    // ack for oxygen that molecule is created
    sem_post(oxygenSem);
    sem_wait(hydrogenSem);

    return;
}

int main(int argc, char **argv)
{
    if (argc != 5)
    {
        fprintf(stderr, "Invalid count of arguments, expected 4 got: %d\n", argc - 1);
        clear();
        exit(EXIT_FAILURE);
    }

    pid_t pid;
    file = fopen("proj2.out", "w");
    long NO, NH, TI, TB;
    int errCount = 0;

    errCount += parseLong(argv[1], &NO);
    errCount += parseLong(argv[2], &NH);
    errCount += parseLong(argv[3], &TI);
    errCount += parseLong(argv[4], &TB);
    errCount += !isValidTime(TI);
    errCount += !isValidTime(TB);

    if (errCount)
    {
        clear();
        exit(EXIT_FAILURE);
    }

    if (init())
    {
        clear();
        exit(EXIT_FAILURE);
    }

    //init of shared memory
    shared = map(shared);
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
            exit(EXIT_SUCCESS);
        }
        else if (pid < 0) // error occured
        {
            fprintf(stderr, "Error while creating hydrogen!");
            clear();
            exit(EXIT_FAILURE);
        }
    }

    // oxygen generator
    for (int id = 1; id <= NO; id++)
    {
        pid = fork();
        if (pid > 0) // child only
        {
            handleOxygen(id, TI, TB);
            exit(EXIT_SUCCESS);
        }
        else if (pid < 0) // error occured
        {
            fprintf(stderr, "Error while creating oxygen!");
            clear();
            exit(EXIT_FAILURE);
        }
    }

    // wait for all kids to die
    for (int i = 0; i < NO + NH; i++)
    {
        wait(NULL);
    }
    clear();
    exit(EXIT_SUCCESS);
}