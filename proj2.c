/**************************************************************/
/*  Project name : VUT-FIT-IOS-Projekt 2                      */
/*  File : proj2.c                                            */
/*  Date : 24.4.2022                                          */
/*  Author : Jakub Mašek xmasek@19stud.fit.vutbr.cz           */
/*  Description : synchronization - H20 creating              */
/**************************************************************/

#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define MAP_ANONYMOUS 0x20

struct shared_t {
    int moleculeID;
    int NoOUsed;  // number of oxygens already used for creating molecules
    int NoHUsed;  // number of hydrogens already used for creating molecules
    int row;
    long NO;
    long NH;
} * shared;

FILE *file;
sem_t *oxyMolecSem = NULL;
sem_t *hydMolecSem = NULL;
sem_t *writeSem = NULL;
sem_t *oxygenSem = NULL;
sem_t *hydrogenSem = NULL;

/**
 * @brief Clear semaphores, memory and file
 *
 */
void clear() {
    sem_close(oxyMolecSem);
    sem_close(hydMolecSem);
    sem_close(writeSem);
    sem_close(oxygenSem);
    sem_close(hydrogenSem);
    sem_unlink("/xmasek19.IOS.Projekt2.oxyMolecSem");
    sem_unlink("/xmasek19.IOS.Projekt2.hydMolecSem");
    sem_unlink("/xmasek19.IOS.Projekt2.writeSem");
    sem_unlink("/xmasek19.IOS.Projekt2.oxygenSem");
    sem_unlink("/xmasek19.IOS.Projekt2.hydrogenSem");
    munmap(shared, sizeof *shared);

    if (file != NULL) {
        fclose(file);
    }
}

/**
 * @brief Parse string to long
 *
 * @param src Source string
 * @param dest Pointer to destination
 */
void parseLong(char *src, long *dest) {
    char *ptr;
    *dest = strtol(src, &ptr, 10);

    if (ptr[0] != '\0') {
        fprintf(stderr, "Unexpected character: %s\n", ptr);
        clear();
        exit(1);
    }

    if (*dest < 0) {
        fprintf(stderr, "Negative numbers are not allowed! Found: %ld\n", *dest);
        clear();
        exit(1);
    }
    return;
}

/**
 * @brief Validates creating time
 *
 * @param time Time to be validated
 * @return int error -> 1 / valid -> 0
 */
int isValidTime(int time) {
    if (time >= 0 && time <= 1000) {
        return 1;
    }
    fprintf(stderr, "Invalid time, expected: 0-1000, got: %d.\n", time);
    return 0;
}

/**
 * @brief Init semaphore
 *
 * @param sem Semaphore to be initialized
 * @param name Name of semaphore
 * @param initState Semaphore starting state
 */
void initSem(sem_t **sem, char *name, int initState) {
    char string[50];
    strcpy(string, "/xmasek19.IOS.Projekt2.");
    strcat(string, name);
    if ((*sem = sem_open(string, O_CREAT | O_EXCL, 0666, initState)) == SEM_FAILED) {
        printf("Semaphore %s not created\n", name);
        clear();
        exit(1);
    }
    return;
}

/**
 * @brief Sleeps for random time between 0 and max time
 *
 * @param max Max time of sleep (ms)
 * @param row Number of current row to randomize random function
 */
void mysleep(int max, int row) {
    if (max == 0) {
        return;
    }
    srand(getpid() / row);
    int time = (rand() % max) + 1;
    usleep(time * 1000);
}

/**
 * @brief Synchronized print to file (atoms)
 *
 * @param string Format string
 * @param shared Shared memory
 * @param atomID ID of current atom
 */
void syncPrintAtom(char string[], struct shared_t *shared, int atomID) {
    sem_wait(writeSem);
    fprintf(file, string, shared->row, atomID);
    fflush(file);
    shared->row++;
    sem_post(writeSem);
}

/**
 * @brief Synchronized print to file (molecules)
 *
 * @param string Format string
 * @param shared Shared memory
 * @param atomID ID of current atom
 */
void syncPrintMolecule(char string[], struct shared_t *shared, int atomID) {
    sem_wait(writeSem);
    fprintf(file, string, shared->row, atomID, shared->moleculeID);
    fflush(file);
    shared->row++;
    sem_post(writeSem);
}

/**
 * @brief Handle hydrogen process
 *
 * @param id ID of atom
 * @param TI Max time to initial atom
 * @param TB Max time to create melecule
 */
void handleOxygen(int id, int TI, int TB) {
    syncPrintAtom("%d: O %d: started\n", shared, id);
    mysleep(TI, shared->row);
    syncPrintAtom("%d: O %d: going to queue\n", shared, id);

    // wait for previous molecule to be created
    sem_wait(oxyMolecSem);

    if (shared->NH - shared->NoHUsed < 2) {
        syncPrintAtom("%d: O %d: not enough H\n", shared, id);
        sem_post(oxyMolecSem);
        return;
    }

    syncPrintMolecule("%d: O %d: creating molecule %d \n", shared, id);

    // wait for both H atoms to start creating
    sem_wait(oxygenSem);
    sem_wait(oxygenSem);
    sem_post(hydrogenSem);
    sem_post(hydrogenSem);

    mysleep(TB, shared->row);

    syncPrintMolecule("%d: O %d: molecule %d created\n", shared, id);

    // wait for both hydrogens to write "molecule x created"
    sem_wait(oxygenSem);
    sem_wait(oxygenSem);

    // no synchronization needed (only 1 process at the same time)
    shared->moleculeID++;
    shared->NoOUsed++;

    // let 1 oxygen and 2 hydrogens to create another molecule
    sem_post(oxyMolecSem);
    sem_post(hydMolecSem);
    sem_post(hydMolecSem);
    return;
}

/**
 * @brief Handle hydrogen process
 *
 * @param id ID of atom
 * @param TI Max time to initial atom
 */
void handleHydrogen(int id, int TI) {
    syncPrintAtom("%d: H %d: started\n", shared, id);
    mysleep(TI, shared->row);
    syncPrintAtom("%d: H %d: going to queue\n", shared, id);

    // wait for previous molecule to be created
    sem_wait(hydMolecSem);

    if (shared->NH - shared->NoHUsed < 2 || shared->NO == shared->NoOUsed) {
        syncPrintAtom("%d: H %d: not enough O or H\n", shared, id);
        sem_post(hydMolecSem);
        return;
    }

    syncPrintMolecule("%d: H %d: creating molecule %d \n", shared, id);

    // wait for all 3 atoms to start creating
    sem_post(oxygenSem);
    sem_wait(hydrogenSem);
    syncPrintMolecule("%d: H %d: molecule %d created\n", shared, id);

    // synchronized operation with shared variable
    sem_wait(writeSem);
    shared->NoHUsed++;
    sem_post(writeSem);

    // ack for oxygen that molecule is created
    sem_post(oxygenSem);
    return;
}

/**
 * @brief Main function
 *
 * @param argc Argument count
 * @param argv Pointer to arguments
 * @return int Exit code
 */
int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "Invalid count of arguments, expected 4 got: %d\n", argc - 1);
        return 1;
    }

    pid_t pid;
    file = fopen("proj2.out", "w");
    long NO, NH, TI, TB;

    // process input
    parseLong(argv[1], &NO);
    parseLong(argv[2], &NH);
    parseLong(argv[3], &TI);
    parseLong(argv[4], &TB);
    if (!isValidTime(TI))
        exit(1);
    if (!isValidTime(TB))
        exit(1);

    // init semaphores
    initSem(&oxygenSem, "oxygenSem", 0);
    initSem(&hydrogenSem, "hydrogenSem", 0);
    initSem(&oxyMolecSem, "oxyMolecSem", 1);
    initSem(&hydMolecSem, "hydMolecSem", 2);
    initSem(&writeSem, "writeSem", 1);

    // init shared memory
    shared = mmap(NULL, sizeof(shared), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    shared->moleculeID = 1;
    shared->row = 1;
    shared->NO = NO;
    shared->NH = NH;

    // hydrogen generator
    for (int id = 1; id <= NH; id++) {
        pid = fork();
        if (pid == 0)  // child only
        {
            handleHydrogen(id, TI);
            return 0;
        } else if (pid < 0)  // error occured
        {
            fprintf(stderr, "Error while creating hydrogen!");
            clear();
            return 1;
        }
    }

    // oxygen generator
    for (int id = 1; id <= NO; id++) {
        pid = fork();
        if (pid == 0)  // child only
        {
            handleOxygen(id, TI, TB);
            return 0;
        } else if (pid < 0)  // error occured
        {
            fprintf(stderr, "Error while creating oxygen!");
            clear();
            return 1;
        }
    }

    // wait for all kids to die
    for (int i = 0; i < NO + NH; i++) {
        wait(NULL);
    }
    clear();
    return 0;
}