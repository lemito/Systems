//
// Created by lemito on 2/24/25.
//
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <unistd.h>

#include "../include/base.h"

#define SEM_NAME "/home/lemito/Desktop/my_sems/my_super_duper_semaphore"

#define LEFT_PORIGE(i, N) (i)  // левая вилка совпадает с номером философа
#define RIGHT_PORIGE(i, N) \
  ((i + 1) % N)  // правая = левая + 1; % N - чтобы было колько вычетов

struct useful_things {
  int semid;     // id  семафора
  int philo_id;  // номер философа
  int cnt;       // кол-во челиксов
};

typedef struct useful_things useful_things;

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
    perror("err: ");
    return SEM_ERR;
  }
  return SUCCESS;
}

#define SEM_WAIT(semid, semix) sem_op((semid), (semix), -1)
#define SEM_POST(semid, semix) sem_op((semid), (semix), +1)

void *WORKER(void *p) {
  if (p == NULL) {
    return NULL;
  }
  const int semid = ((useful_things *)p)->semid;
  const int philo_id = ((useful_things *)p)->philo_id;
  const int N = ((useful_things *)p)->cnt;

  int max_cycle = 3;

  while (max_cycle) {
    printf("Философ %d оч умно думает(он философ)\n", philo_id);
    sleep(1);

    printf("Философ %d надумался и хочет есть\n", philo_id);

    /*
     * 01 взяли; опустили
     * 12 взяли; отпустили
     * ...
     *
     * если все берут вилки в одном порядке, то обязательно по кругу встретяться
     * и заблочат друг друга; поэтому надо делать так, что четные берут сначала
     * правую, а четные - сначала леву, так есть шанс что хоть кто-то возьмет

     семафор - "официант", регулирует как именно будут взяты вилки (нельзя брать
     ту, которая уже у кого-то)
     */
    if (philo_id & 1) {
      if (SUCCESS != SEM_WAIT(semid, RIGHT_PORIGE(philo_id, N))) {
        printf("Не удалось взять вилку\n");
      }

      if (SUCCESS != SEM_WAIT(semid, LEFT_PORIGE(philo_id, N))) {
        printf("Не удалось взять вилку\n");
      }
    } else {
      if (SUCCESS != SEM_WAIT(semid, LEFT_PORIGE(philo_id, N))) {
        printf("Не удалось взять вилку\n");
        return NULL;
      }
      if (SUCCESS != SEM_WAIT(semid, RIGHT_PORIGE(philo_id, N))) {
        printf("Не удалось взять вилку\n");
      }
    }
    printf("Нямно покушал %d\n", philo_id);
    sleep(1);

    if (SUCCESS != SEM_POST(semid, LEFT_PORIGE(philo_id, N))) {
      printf("Не удалось скинуть вилку\n");
    }
    if (SUCCESS != SEM_POST(semid, RIGHT_PORIGE(philo_id, N))) {
      printf("Не удалось скинуть вилку\n");
    }
    printf("Закончили с %d, он снова думает. Идем дальше\n", philo_id);
    max_cycle--;
  }

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


V F V F V F ->

-1 | -1 | sv |
sv | -1 | -1 |
-1 | sv | -1 |

все поели, все подумали => все счастливы
*/
int main(void) {
  // семафор - официант
  const key_t sem_key = IPC_PRIVATE;
  int semid = 0;
  int PHILOSOPHER_CNT = 0;  // колво семафоров/потоков(и философов)
  int st = 0;
  pthread_t *philosophers = NULL;  // стол философов
  useful_things *args = NULL;      // мета инфа

  srand(time(NULL));

  for (;;) {
    printf("Сколько философов сидят за столом? >> ");
    st = scanf("%d", &PHILOSOPHER_CNT);
    if (st != 1) {
      printf("Введи натуральное целое чисоло\n");
      // return INPUT_ERROR;
    }
    if (PHILOSOPHER_CNT <= 0) {
      printf("Введи натуральное целое чисоло\n");
      // return INPUT_ERROR;
    }

    break;
  }

  philosophers = (pthread_t *)malloc(PHILOSOPHER_CNT * sizeof(pthread_t));
  if (philosophers == NULL) {
    return MEMORY_ERROR;
  }

  args = (useful_things *)malloc(PHILOSOPHER_CNT * sizeof(useful_things));
  if (args == NULL) {
    FREE_AND_NULL(philosophers);
    return MEMORY_ERROR;
  }

  if ((semid = semget(sem_key, PHILOSOPHER_CNT, IPC_CREAT | 0666)) == -1) {
    printf("Ошибка создания семафора\n");
    FREE_AND_NULL(philosophers);
    FREE_AND_NULL(args);
    return SEM_ERR;
  }

  for (int philo_id = 0; philo_id < PHILOSOPHER_CNT; philo_id++) {
    union semun arg;
    arg.val = 1;
    st = semctl(semid, philo_id, SETVAL, arg);
    if (st == -1) {
      return SEM_ERR;
    }
    // semctl(semid, philo_id, SETVAL, 1);
  }

  for (int philo_id = 0; philo_id < PHILOSOPHER_CNT; philo_id++) {
    args[philo_id].semid = semid;
    args[philo_id].cnt = PHILOSOPHER_CNT;
    args[philo_id].philo_id = philo_id;
    st = pthread_create(philosophers + philo_id, NULL, WORKER, args + philo_id);
    if (st == -1) {
      return MEMORY_ERROR;
    }
  }

  for (int philo_id = 0; philo_id < PHILOSOPHER_CNT; philo_id++) {
    st = pthread_join(philosophers[philo_id], NULL);
    if (st == -1) {
      return MEMORY_ERROR;
    }
  }

  if (-1 == semctl(semid, PHILOSOPHER_CNT, IPC_RMID)) {
    printf("Не удалось удалить семафор\n");
    FREE_AND_NULL(philosophers);
    FREE_AND_NULL(args);
    return SEM_ERR;
  }

  st = semctl(semid, 0, IPC_RMID, 0);
  if (st == -1) {
    return MEMORY_ERROR;
  }
  FREE_AND_NULL(philosophers);
  FREE_AND_NULL(args);

  return 0;
}
