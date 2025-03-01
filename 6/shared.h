//
// Created by lemito on 2/27/25.
//

#ifndef SHARED_H
#define SHARED_H
#include <stdio.h>
#include <sys/sem.h>
#define MAXSETSIZE 2

#include "../include/base.h"

// тут живут пути всех вещей
#define SHM_NAME "/tmp/my_beutiful_shm"
#define SEM_NAME "/tmp/my_beutiful_sem"
#define SHM_RESULT_NAME "/tmp/my_beutiful_shm_result"
#define INFO_NAME "/tmp/info_size"

#define SIZE_SIZE sizeof(size_t)

#define CLIENT_IX 0
#define SERVER_ix 1
#define SIZE_SERVER_ix 2
#define SIZE_CLIENT_ix 3
#define SERVER_RES_IX 4
#define CLIENT_RES_IX 5

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
  sem.sem_num = sem_ix;  // над кем будем творить
  sem.sem_op = op;       // что будем творить
  sem.sem_flg = 0;
  // собственно, творим
  if (-1 == semop(semid, &sem, 1)) {
    perror("err (sem_op): ");
    return SEM_ERR;
  }
  return SUCCESS;
}

// semix ждет
#define SEM_WAIT(semid, semix) sem_op((semid), (semix), -1)
// говорим semix, что мы готовы
#define SEM_POST(semid, semix) sem_op((semid), (semix), 1)

#define INIT(semid)

#endif  // SHARED_H
