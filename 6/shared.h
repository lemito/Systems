//
// Created by lemito on 2/27/25.
//

#ifndef SHARED_H
#define SHARED_H
#include "../include/base.h"

#include <stdio.h>
#include <sys/sem.h>

#define SHM_NAME "/tmp/my_beutiful_shm"
#define SEM_NAME "/tmp/my_beutiful_sem"

#define CLEAR(ptr, n) memset(ptr, 0, n)


/**
 * удобная штучка для управление семафорчиком
 * @param semid id-шник семафора
 * @param sem_ix порядковый номер семафора
 * @param op то, что нужно прибавить
 * @return always SUCCESS
 */
STATUS_CODE sem_op(const int semid, const int sem_ix, const short op) {
    struct sembuf sem;
    sem.sem_num = sem_ix; // над кем будем творить
    sem.sem_op = op; // что будем творить
    sem.sem_flg = 0;
    // собственно, творим
    if (-1 == semop(semid, &sem, 1)) {
        perror("err: ");
        return SEM_ERR;
    }
    return SUCCESS;
}

#define SEM_WAIT(semid, semix) sem_op((semid), (semix), -1)
#define SEM_POST(semid, semix) sem_op((semid), (semix), +1)

#endif //SHARED_H
