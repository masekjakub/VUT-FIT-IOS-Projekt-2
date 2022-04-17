#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

#define map(var) (mmap(NULL, sizeof(*var), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));

sem_t *semaphorOxy = NULL;
sem_t *semaphorHyd = NULL;
sem_t *writeSem = NULL;
sem_t *moleculeSem = NULL;
sem_t *moleculeDoneSem = NULL;

FILE *outF;
struct shared_t
{
    int row;
    int numOfOxy;
    int numOfHyd;
    int creating;
    long no;
    long nh;

};

struct shared_t *shared = NULL;
int *moleculeId = NULL;

    int
    parseInt(char *src, long *dest)
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
    sem_close(writeSem);
    sem_close(moleculeSem);
    sem_close(moleculeDoneSem);
    sem_unlink("/xmasek19.IOS.Projekt2.O");
    sem_unlink("/xmasek19.IOS.Projekt2.H");
    sem_unlink("/xmasek19.IOS.Projekt2.Write");
    sem_unlink("/xmasek19.IOS.Projekt2.Molecule");
    sem_unlink("/xmasek19.IOS.Projekt2.MoleculeDone");
    fclose(stdout);
}

int init()
{
    // outF = fopen("proj2.out", "w");
    if ((semaphorOxy = sem_open("/xmasek19.IOS.Projekt2.O", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        printf("Semaphore O not created\n");
        return EXIT_FAILURE;
    }
    if ((semaphorHyd = sem_open("/xmasek19.IOS.Projekt2.H", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {
        printf("Semaphore H not created\n");
        return EXIT_FAILURE;
    }
    if ((writeSem = sem_open("/xmasek19.IOS.Projekt2.Write", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        printf("Semaphore write not created\n");
        return EXIT_FAILURE;
    }
    if ((moleculeSem = sem_open("/xmasek19.IOS.Projekt2.Molecule", O_CREAT | O_EXCL, 0666, 2)) == SEM_FAILED)
    {
        printf("Semaphore molecule not created\n");
        return EXIT_FAILURE;
    }
    if ((moleculeDoneSem = sem_open("/xmasek19.IOS.Projekt2.MoleculeDone", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {
        printf("Semaphore moleculeDone not created\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// FIX ME returns same time
void mysleep(int max)
{
    int time = (rand() % max) + 1;
    // printf("Time: %d!\n",time);
    usleep(time);
    return;
}
void handleOxygen(int id, int ti, int tb)
{
    sem_wait(writeSem);
    printf("%d: O %d: started\n", shared->row += 1, id);
    sem_post(writeSem);
    shared->numOfOxy++;

    mysleep(ti);
    sem_wait(writeSem);
    printf("%d: O %d: going to queue\n", shared->row += 1, id);
    sem_post(writeSem);

    //wait until other molecule is created
    sem_wait(semaphorOxy);
    sem_wait(moleculeSem);
    printf("Ox: %d\n", shared->numOfOxy);
    sem_wait(moleculeSem);
    //start creating
    if (shared->numOfHyd<2){
        sem_wait(writeSem);
        printf("%d: O %d: not enough H \n", shared->row += 1, id);
        sem_post(writeSem);
        sem_post(semaphorHyd);
        exit(0);
    }
    sem_wait(writeSem);
    printf("%d: O %d: creating molecule %d\n", shared->row += 1, id, *moleculeId += 1);
    sem_post(writeSem);
    shared->numOfOxy--;
    shared->creating = 2;

    sem_post(semaphorHyd);
    sem_post(semaphorHyd);
    mysleep(tb);
    //created molecule
    sem_wait(writeSem);
    printf("%d: O %d: molecule %d created\n", shared->row += 1, id, *moleculeId);
    sem_post(writeSem);

    sem_post(moleculeDoneSem);
    sem_post(moleculeDoneSem);

    exit(0);
}

void handleHydrogen(int id, int ti, int tb)
{
    sem_wait(writeSem);
    printf("%d: H %d: started\n", shared->row += 1, id);
    sem_post(writeSem);
    shared->numOfHyd++;

    mysleep(ti);
    sem_wait(writeSem);
    printf("%d: H %d: going to queue\n", shared->row += 1, id);
    sem_post(writeSem);
    sem_wait(semaphorHyd);

    // start creating
    if (shared->numOfHyd < 2 || shared->numOfOxy == 0)
    {
            sem_wait(writeSem);
            printf("%d: O %d: not enough O or H \n", shared->row += 1, id);
            sem_post(writeSem);
            exit (0);
    }
    sem_wait(writeSem);
    printf("%d: H %d: creating molecule %d\n", shared->row += 1, id, *moleculeId);
    sem_post(writeSem);
    shared->numOfHyd--;

    sem_wait(moleculeDoneSem);
    sem_wait(writeSem);
    printf("%d: H %d: molecule %d created\n", shared->row += 1, id, *moleculeId);
    sem_post(writeSem);

    sem_post(moleculeSem);
    shared->creating--;
    if (shared->creating == 0)
    {
        sem_post(semaphorOxy);
    }

    exit(0);
}

int main(int argc, char **argv)
{
    clear();
    freopen("proj2.out", "w", stdout);
    setbuf(stdout, NULL);

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
    shared = map(shared);
    moleculeId = map(moleculeId);
    shared->row = 0;
    shared->no = no;
    shared->nh = nh;
    *moleculeId = 0;
    if (init())
    {
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i <= nh; i++)
    {
        pid = fork();
        if (pid == 0) // only child
        {
            handleHydrogen(i, ti, tb);
        }
        else if (pid < 0) // error occured
        {
            fprintf(stderr, "Error while creating hydrogen!");
            clear();
            return EXIT_FAILURE;
        }
    }

    for (int i = 1; i <= no; i++)
    {
        pid = fork();
        if (pid == 0) // only child
        {
            handleOxygen(i, ti, tb);
        }
        else if (pid < 0) // error occured
        {
            fprintf(stderr, "Error while creating oxygen!");
            return EXIT_FAILURE;
        }
    }
    munmap(shared, sizeof *shared);
    exit(0);

    sleep(1);
    int num;
    sem_getvalue(semaphorOxy, &num);
    printf("No O start: %d\n", num);

    sem_getvalue(semaphorOxy, &num);
    printf("No O end: %d\n", num);
    sleep(1);
    sem_getvalue(semaphorOxy, &num);
    printf("No O end: %d\n", num);
    printf("Row: %d\n", shared->row);

    
    clear();
    printf("End\n");
    exit(0);
}