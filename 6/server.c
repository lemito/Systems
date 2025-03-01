#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "shared.h"

#define MAX_SIZE (4096 * 10)

STATUS_CODE dir_view(char *path) {
  if (path == NULL) {
    return INPUT_ERROR;
  }
  return SUCCESS;
}

int main(void) {
  key_t skey, semkey, sizekey;
  int shm;
  int sem;
  int size_id;
  void *meow;
  size_t *info = NULL;

  if ((sizekey = ftok(INFO_NAME, 'S')) == -1) {
    perror("ftok sizekey\n");
  }
  if ((skey = ftok(SHM_NAME, 'S')) == -1) {
    perror("ftok shm\n");
  }
  if ((semkey = ftok(SEM_NAME, 'b')) == -1) {
    perror("ftok sem\n");
  }
  size_id = shmget(sizekey, sizeof(size_t), IPC_CREAT | 0666);
  if (size_id != -1) {
    semctl(size_id, 0, IPC_RMID);
    return MEMORY_ERROR;
  }

  {
    int existing_sem = semget(semkey, 2, 0666);
    if (existing_sem != -1) {
      semctl(existing_sem, 0, IPC_RMID);
    }
  }

  sem = semget(semkey, 2, IPC_CREAT | IPC_EXCL | 0666);
  if (sem == -1) {
    perror("semget");
    return SEM_ERR;
  }
  {
    struct semid_ds ds;
    union semun arg;
    arg.buf = &ds;
    if (semctl(sem, 0, IPC_STAT, arg) == -1) {
      perror("semctl IPC_STAT");
    } else {
      printf("Создан набор семафоров, количество элементов: %lu\n",
             ds.sem_nsems);
    }
  }
  union semun arg;
  arg.val = 0;
  if (semctl(sem, SERVER_ix, SETVAL, arg.val) == -1) {
    perror("semctl SETVAL SERVER_ix");
    return SEM_ERR;
  }
  arg.val = 0;
  if (semctl(sem, CLIENT_IX, SETVAL, arg.val) == -1) {
    perror("semctl SETVAL CLIENT_IX");
    return SEM_ERR;
  }

  if ((shm = shmget(skey, MAX_SIZE, IPC_CREAT | 0666)) == -1) {
    perror("shmget data\n");
    return MEMORY_ERROR;
  }

  if ((meow = shmat(shm, NULL, 0)) == (void *)-1) printf("shmat");
  CLEAR(meow, MAX_SIZE);

  printf("Сервер запустился!!!\n");

  while (1) {
    // printf("%d %d %d\n", sem, SERVER_ix, semctl(sem, SERVER_ix, GETVAL, 0));
    if (SEM_WAIT(sem, SERVER_ix) != SUCCESS) {
      fprintf(stderr, "Ошибка SEM_WAIT SERVER_ix\n");
      //   continue;
    }

    info = shmat(size_id, NULL, 0);
    if (info == (size_t*)-1) {
      perror("shmget info\n");
      return MEMORY_ERROR;
    }


    size_t off = 0;

    const size_t *sPtr = (size_t *)meow;
    const char *mPtr = (char *)meow + sizeof(size_t);

    if (*sPtr == 0) {
      printf("Сервер получил сигнал завершения. Выход...\n");
    } else {
      printf("Сервер начал обработку\n");

      printf("Путей получено = %lu\n", *sPtr);
      for (size_t i = 0; i < *sPtr; i++) {
        printf("path=%s\n", mPtr + off);
        dir_view(mPtr + off);
        off += strlen(mPtr + off) + 1;
      }
    }

    CLEAR(meow, MAX_SIZE);

    if (SEM_POST(sem, CLIENT_IX) != SUCCESS) {
      fprintf(stderr, "Ошибка SEM_POST CLIENT_IX\n");
    } else {
      printf("Сервер отработал клиента!\n");
    }
  }

  if (shmdt(meow) == -1) {
    printf("shmdt");
  }
  semctl(sem, 0, IPC_RMID);

  return 0;
}
