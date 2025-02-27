#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "shared.h"

int main(const int argc, char **argv) {
    key_t skey, semkey;
    int shm;
    int sem;
    int *meow;

    // if (argc != 2) {
    //   return INPUT_ERROR;
    // }

    for (size_t i = 1; i < argc; i++) {
        if (argv[i] == NULL) {
            return INPUT_ERROR;
        }
    }

    if ((skey = ftok(SHM_NAME, 'S')) == -1) { perror("ftok shm\n"); }
    if ((semkey = ftok(SEM_NAME, 'S')) == -1) { perror("ftok sem\n"); }
    if ((shm = shmget(skey, sizeof(int), IPC_CREAT | 0666)) == -1) {
        return MEMORY_ERROR;
    }
    if ((sem = semget(semkey, 1, IPC_CREAT | 0666)) == -1) {
        printf("Ошибка создания семафора\n");
        return SEM_ERR;
    }
    if ((meow = (int *) shmat(shm, NULL, 0)) == (int *) -1)
        printf("shmat");
    CLEAR(meow, sizeof(int));

    *meow = 52;

    if (shmdt(meow) == -1) printf("shmdt");
    semctl(sem, 0, IPC_RMID);

    return 0;
}
