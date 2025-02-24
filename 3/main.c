//
// Created by lemito on 2/24/25.
//
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>

#include "../include/base.h"

#define SEM_NAME "/my_super_duper_semaphore"

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
    return SEM_ERR;
  }
  return SUCCESS;
}

void *WORKER(void *p) {
  printf("Do some work\nWork work work\n");
  return NULL;
}

/*
2 филосова, 2 вилки
Кушать == 2 вилки (поч 2? Вроде одна рука с вилкой, другая для ножа...)
Думать == 0 вилок
Поток - философ
Семафор(ы) - вилки

Забрать семафор === забрать вилку
Вилка слева и справа от филосовфа
id?


0 1 2 3
F V F V ->
*/
int main(void) {
  int semid = 0;
  int PHILOSOPHER_CNT = 0;
  int st = 0;
  pthread_t *philosophers = NULL;  // стол философов
  int *purege = NULL;              // ложечки у философов

  for (;;) {
    printf("Сколько философов сидят за столом? >> ");
    st = scanf("%d", &PHILOSOPHER_CNT);
    if (st != 1) {
      printf("Введи натуральное целое чисоло\n");
      return INPUT_ERROR;
    }
    if (PHILOSOPHER_CNT <= 0) {
      printf("Введи натуральное целое чисоло\n");
      return INPUT_ERROR;
    }

    break;
  }

  philosophers = (pthread_t *)malloc(PHILOSOPHER_CNT * sizeof(pthread_t));
  if (philosophers == NULL) {
    return MEMORY_ERROR;
  }
  purege = (int *)malloc(PHILOSOPHER_CNT * sizeof(int));
  if (purege == NULL) {
    free(philosophers);
    return MEMORY_ERROR;
  }

  if ((semid = semget(IPC_PRIVATE, PHILOSOPHER_CNT, IPC_CREAT | IPC_EXCL)) ==
      -1) {
    printf("Ошибка создания семафора\n");
    free(philosophers);
    free(purege);
    return SEM_ERR;
  }

  for (int philo_id = 0; philo_id < PHILOSOPHER_CNT; philo_id++) {
    semctl(semid, philo_id, SETVAL, 1);
  }

  for (int philo_id = 0; philo_id < PHILOSOPHER_CNT; philo_id++) {
    purege[philo_id] = philo_id;
    pthread_create(philosophers + philo_id, NULL, WORKER, NULL);
  }

  for (int philo_id = 0; philo_id < PHILOSOPHER_CNT; philo_id++) {
    pthread_join(philosophers[philo_id], NULL);
  }

  if (-1 == semctl(semid, PHILOSOPHER_CNT, IPC_RMID)) {
    printf("Не удалось удалить семафор\n");
    free(philosophers);
    free(purege);
    return SEM_ERR;
  }

  free(philosophers);
  free(purege);

  return 0;
}
