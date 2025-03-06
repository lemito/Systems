#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <unistd.h>

#include "../include/base.h"

#define SEM_NAME "/home/lemito/Desktop/my_sems/bath_sem"

#define MUTEX 0
#define MAN 1
#define WOMAN 2

typedef struct data {
  int semid;
  char state;  // M - man, W - woman, F - free
  int N;
  int cur_cnt;
} data_t;

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

STATUS_CODE woman_wants_to_enter(data_t* p) {
  if (p == NULL) {
    return NULL_PTR;
  }
  SEM_WAIT(p->semid, MUTEX);

  while (p->state == 'M' || p->cur_cnt >= p->N) {
    SEM_POST(p->semid, MUTEX);  // разблок мьютекса
    SEM_WAIT(p->semid, WOMAN);  // так уж и быть, отдаем ванную комнату мужикам
    SEM_WAIT(p->semid, MUTEX);  // блок
  }

  p->cur_cnt++;
  p->state = 'W';
  printf("Женщина зашла в ванную комнату, теперь женщин - %d\n", p->cur_cnt);

  SEM_POST(p->semid, MUTEX);
  return SUCCESS;
}

STATUS_CODE man_wants_to_enter(data_t* p) {
  if (p == NULL) {
    return NULL_PTR;
  }
  SEM_WAIT(p->semid, MUTEX);

  while (p->state == 'W' || p->cur_cnt >= p->N) {
    SEM_POST(p->semid, MUTEX);  // разблок мьютекса
    SEM_WAIT(p->semid, MAN);  // так уж и быть, отдаем ванную комнату мужикам
    SEM_WAIT(p->semid, MUTEX);  // блок
  }

  p->cur_cnt++;
  p->state = 'M';
  printf("Сигма зашел в ванную комнату, теперь сигм - %d\n", p->cur_cnt);

  SEM_POST(p->semid, MUTEX);
  return SUCCESS;
}

STATUS_CODE woman_leaves(data_t* p) {
  if (p == NULL) {
    return NULL_PTR;
  }
  SEM_WAIT(p->semid, MUTEX);

  p->cur_cnt--;
  printf("Женщина вышла из душа, сейчас там женщин - %d\n", p->cur_cnt);

  if (p->cur_cnt == 0) {
    p->state = 'F';
    // SEM_POST(p->semid, MAN);

    // сбрасываем все до заводских
    // semctl(p->semid, MUTEX, SETVAL, 1);
    semctl(p->semid, MAN, SETVAL, p->N);
    // semctl(p->semid, WOMAN, SETVAL, 0);
    // for (int i = 0; i < p->N; i++) {
    //   SEM_POST(p->semid, MAN);
    // }
  }

  SEM_POST(p->semid, MUTEX);
  return SUCCESS;
}

STATUS_CODE man_leaves(data_t* p) {
  if (p == NULL) {
    return NULL_PTR;
  }
  SEM_WAIT(p->semid, MUTEX);

  p->cur_cnt--;
  printf("Сигма вышел из душа, сейчас там сигм - %d\n", p->cur_cnt);

  if (p->cur_cnt == 0) {
    p->state = 'F';
    // SEM_POST(p->semid, WOMAN);

    // сбрасываем все до заводских
    semctl(p->semid, WOMAN, SETVAL, p->N);

    //   for (int i = 0; i < p->N; i++) {
    //     SEM_POST(p->semid, WOMAN);
    // }
  }

  SEM_POST(p->semid, MUTEX);
  return SUCCESS;
}

void* work(void* p) {
  if (p == NULL) {
    return NULL;
  }

  // TODO: мб сделать чтобы не случайно выбирался пол, а из массива
  // char cur_gender = (rand() % 2 == 0) ? 'M' : 'W';
  data_t* targ = (data_t*)p;
  // switch (cur_gender) {
  //   case 'M': {
  man_wants_to_enter(targ);
  sleep(rand() % 3);
  man_leaves(targ);
  //   } break;
  //   case 'W': {
  //     woman_wants_to_enter(targ);
  //     sleep(rand() % 3);
  //     woman_leaves(targ);
  //   } break;
  // }
  // printf("bbb\n");
  return NULL;
}

void* work2(void* p) {
  if (p == NULL) {
    return NULL;
  }
  data_t* targ = (data_t*)p;
  // TODO: мб сделать чтобы не случайно выбирался пол, а из массива
  // char cur_gender = (rand() % 2 == 0) ? 'M' : 'W';
  // data_t* targ = (data_t*)p;
  // switch (cur_gender) {
  //   case 'M': {
  //     man_wants_to_enter(targ);
  //     sleep(rand() % 3);
  //     man_leaves(targ);
  //   } break;
  //   case 'W': {
  woman_wants_to_enter(targ);
  sleep(rand() % 3);
  woman_leaves(targ);
  //   } break;
  // }
  // printf("bbb\n");
  return NULL;
}

/*
семафоры:
<общий> <man> <woman> => 3

*/
int main(int argc, char const* argv[]) {
  int N, PEOPLES;  // N - maxлюдей, PEOPLES - все люди
  int st;          // st - проверки
  key_t semkey;    // ключ сема
  int semid;
  pthread_t* sim;  // потоки симуляции
  data_t targ;     // аргумент потока
  int man, woman;

  srand(time(NULL));

  // printf("Введи общее количество человек >> ");
  // st = scanf("%d", &PEOPLES);
  // if (st != 1) {
  //   printf("Невалидное число - ожидалось int");
  //   return INPUT_ERROR;
  // }
  // if (PEOPLES <= 0) {
  //   printf("Должно быть людей >0\n");
  //   return INPUT_ERROR;
  // }
  printf("Введи общее количество мужчин >> ");
  st = scanf("%d", &man);
  if (st != 1) {
    printf("Невалидное число - ожидалось int");
    return INPUT_ERROR;
  }
  if (man <= 0) {
    printf("Должно быть людей >0\n");
    return INPUT_ERROR;
  }
  printf("Введи общее количество женщин >> ");
  st = scanf("%d", &woman);
  if (st != 1) {
    printf("Невалидное число - ожидалось int");
    return INPUT_ERROR;
  }
  if (woman <= 0) {
    printf("Должно быть людей >0\n");
    return INPUT_ERROR;
  }

  printf("Введи MAX количество кабинок в ванне >> ");
  st = scanf("%d", &N);
  if (st != 1) {
    printf("Невалидное число - ожидалось int");
    return INPUT_ERROR;
  }
  if (N <= 0) {
    printf("Должно быть людей >0\n");
    return INPUT_ERROR;
  }

  semkey = ftok(SEM_NAME, 'l');
  if (semkey == -1) {
    perror("ftok\n");
    return SEM_ERR;
  }

  semid = semget(semkey, 3, IPC_CREAT | 0666);
  if (semid == -1) {
    printf("semget\n");
    return SEM_ERR;
  }
  union semun arg;
  arg.val = 1;
  semctl(semid, MUTEX, SETVAL, arg);
  for (size_t i = 1; i < 3; i++) {
    union semun arg;
    arg.val = 0;
    semctl(semid, i, SETVAL, arg);
  }

  PEOPLES = man + woman;

  sim = (pthread_t*)malloc(sizeof(pthread_t) * PEOPLES);
  if (sim == NULL) {
    semctl(semid, 0, IPC_RMID, 0);
    return MEMORY_ERROR;
  }

  targ.N = N;
  targ.semid = semid;
  targ.state = 'F';
  targ.cur_cnt = 0;

  /*
  3-m 5-woman
  mmmwwwww
  01234567

  */

  for (size_t i = 0; i < man; i++) {
    st = pthread_create(sim + i, NULL, work, &targ);
    if (st == -1) {
      return THREAD_ERROR;
    }
  }
  for (size_t i = man; i < PEOPLES; i++) {
    st = pthread_create(sim + i, NULL, work2, &targ);
    if (st == -1) {
      return THREAD_ERROR;
    }
  }

  for (size_t i = 0; i < PEOPLES; i++) {
    st = pthread_join(sim[i], NULL);
    if (st == -1) {
      return THREAD_ERROR;
    }
  }

  FREE_AND_NULL(sim);
  semctl(semid, 0, IPC_RMID, 0);
  return 0;
}
