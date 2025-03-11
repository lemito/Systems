
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

#include "shared.h"

STATUS_CODE dir_view(const char *path, char **res, size_t *res_size) {
  if (path == NULL) {
    return INPUT_ERROR;
  }

  const char *start_name = strrchr(path, '/');
  if (start_name == NULL) {
    return INPUT_ERROR;
  }

  const size_t siz = start_name - path;
  char result[siz + 1];
  strncpy(result, path, siz);
  result[siz] = '\0';

  DIR *dirp = NULL;              // тек. директория
  struct dirent *dentry = NULL;  // инфа о ней

  if ((dirp = opendir(result)) == NULL) {
    perror("opendir ошибка\n");
    closedir(dirp);
    return ERROR_OPEN;
  }

  size_t cap = 30 + 14;
  char *BUF = malloc(30 + 7 + 7);
  if (BUF == NULL) {
    closedir(dirp);
    return MEMORY_ERROR;
  }
  size_t off = 0;
  off += sprintf(off + BUF, ">>> Сейчас смотрим: %s\n", result);

  while ((dentry = readdir(dirp)) != NULL) {
    char name[PATH_MAX];
    snprintf(name, sizeof(name) / sizeof(char), "%s/%s", result,
             dentry->d_name);
    const size_t for_add = strlen(name) + 45 + off;
    if (for_add >= cap) {
      char *tmp = realloc(BUF, cap + for_add);
      if (tmp == NULL) {
        FREE_AND_NULL(BUF);
        closedir(dirp);
        return MEMORY_ERROR;
      }
      BUF = tmp;
      cap += for_add;
    }
    off += sprintf(BUF + off, "-> ");
    // printf("%s\n", name);
    switch (dentry->d_type) {
      case DT_DIR: {
        off += sprintf(BUF + off, "директория: %s ino = %lu\n", name,
                       dentry->d_ino);
      } break;
      case DT_REG: {
        off += sprintf(BUF + off, "файл: %s ino = %lu\n", name, dentry->d_ino);
      } break;
      case DT_LNK: {
        off += sprintf(BUF + off, "симв. ссылка: %s ino = %lu\n", name,
                       dentry->d_ino);
      } break;
      case DT_UNKNOWN: {
        off += sprintf(BUF + off, "?: %s ino = %lu\n", name, dentry->d_ino);
      } break;
      case DT_BLK: {
        off += sprintf(BUF + off, "диск (блочное устройство): %s ino = %lu\n",
                       name, dentry->d_ino);
      } break;
      case DT_CHR: {
        off +=
            sprintf(BUF + off, "диск (символьное устройство): %s ino = %lu\n",
                    name, dentry->d_ino);
      } break;
      case DT_FIFO: {
        off += sprintf(BUF + off, "именнованный канал: %s ino = %lu\n", name,
                       dentry->d_ino);
      } break;
      case DT_SOCK: {
        off += sprintf(BUF + off, "сокет: %s ino = %lu\n", name, dentry->d_ino);
      } break;
      default: {
        fprintf(stderr, "err");
      } break;
    }
  }
  off += sprintf(BUF + off, "===\n");
  *res = BUF;
  *res_size = strlen(BUF);
  closedir(dirp);
  return SUCCESS;
}

int main(void) {
  key_t skey, semkey, sizekey, reskey;
  size_t *info = NULL;

  if ((sizekey = ftok(INFO_NAME, 'S')) == -1) {
    fprintf(stderr, "ftok sizekey\n");
  }
  if ((skey = ftok(SHM_NAME, 'p')) == -1) {
    fprintf(stderr, "ftok shm\n");
  }
  if ((semkey = ftok(SEM_NAME, 'W')) == -1) {
    fprintf(stderr, "ftok sem\n");
  }
  if ((reskey = ftok(SHM_RESULT_NAME, 'p')) == -1) {
    fprintf(stderr, "ftok sem\n");
  }

  const int size_id = shmget(sizekey, sizeof(size_t), IPC_CREAT | 0666);
  if (size_id == -1) {
    fprintf(stderr, "shmget sizeid");
    return MEMORY_ERROR;
  }
  info = shmat(size_id, NULL, 0);
  if (info == (size_t *)-1) {
    fprintf(stderr, "shmat info");
    return MEMORY_ERROR;
  }
  CLEAR(info, sizeof(size_t));
  {
    const int existing_sem = semget(semkey, 6, 0666);
    if (existing_sem != -1) {
      semctl(existing_sem, 0, IPC_RMID);
    }
  }
  const int sem = semget(semkey, 6, IPC_CREAT | IPC_EXCL | 0666);
  if (sem == -1) {
    fprintf(stderr, "semget");
    return SEM_ERR;
  }
  {
    struct semid_ds ds;
    union semun arg;
    arg.buf = &ds;
    if (semctl(sem, 0, IPC_STAT, arg) == -1) {
      fprintf(stderr, "semctl");
    } else {
      printf("Создан набор семафоров, количество элементов: %lu\n",
             ds.sem_nsems);
    }
  }
  for (size_t i = 0; i < 6; i++) {
    union semun arg;
    arg.val = 0;
    if (semctl(sem, (int)i, SETVAL, arg) == -1) {
      fprintf(stderr, "semctl SETVAL");
      return SEM_ERR;
    }
  }

  printf("Сервер запущен..\n");

  while (1) {
    if (SEM_WAIT(sem, SIZE_SERVER_ix) != SUCCESS) {
      fprintf(stderr, "Ошибка SEM_WAIT SIZE_SERVER_ix\n");
      continue;
    }
    const size_t req_size = *info;
    printf("Сервер: получен размер входных данных: %lu\n", req_size);
    if (SEM_POST(sem, SIZE_CLIENT_ix) != SUCCESS) {
      fprintf(stderr, "Ошибка SEM_POST SIZE_CLIENT_ix\n");
      continue;
    }

    if (SEM_WAIT(sem, SERVER_ix) != SUCCESS) {
      fprintf(stderr, "Ошибка SEM_WAIT SERVER_ix\n");
      continue;
    }
    const int shm = shmget(skey, req_size, IPC_CREAT | 0666);
    if (shm == -1) {
      fprintf(stderr, "shmget data");
      continue;
    }
    void *meow = shmat(shm, NULL, 0);
    if (meow == (void *)-1) {
      fprintf(stderr, "shmat data");
      continue;
    }
    const size_t *sPtr = (size_t *)meow;
    char *mPtr = (char *)meow + sizeof(size_t);
    printf("Сервер: получено путей: %lu\n", *sPtr);
    if (*sPtr == 0) {
      printf("Сервер: клиент передал пустой запрос, пропускаем...\n");
      shmdt(meow);
      shmctl(shm, IPC_RMID, NULL);
      continue;
    }

    // printf("===========%s\n", mPtr);

    char *res = NULL;
    size_t res_size = 0;
    size_t off = 0;
    for (size_t i = 0; i < *sPtr; i++) {
      char *pre_res = NULL;
      size_t len = 0;
      printf("Сервер: обрабатываем путь: %s\n", mPtr + off);
      if (dir_view(mPtr + off, &pre_res, &len) != SUCCESS) {
        fprintf(stderr, "dir_view\n");
        off += strlen(mPtr + off) + 1;
        continue;
      }
      char *tmp = realloc(res, res_size + len + 1);
      if (tmp == NULL) {
        fprintf(stderr, "realloc");
        FREE_AND_NULL(res);
        FREE_AND_NULL(pre_res);
        break;
      }
      res = tmp;
      if (res_size == 0) {
        strcpy(res, pre_res);
      } else {
        strcat(res, pre_res);
      }
      res_size = strlen(res);
      FREE_AND_NULL(pre_res);
      off += strlen(mPtr + off) + 1;
    }
    shmdt(meow);
    shmctl(shm, IPC_RMID, NULL);

    const size_t res_shm_size = sizeof(size_t) + res_size + 1;
    const int cleaner = shmget(reskey, res_shm_size, 0666);
    if (cleaner != -1) {
      shmctl(cleaner, IPC_RMID, NULL);
    }
    const int res_id =
        shmget(reskey, res_shm_size, IPC_CREAT | IPC_EXCL | 0666);
    if (res_id == -1) {
      fprintf(stderr, "shmget result");
      FREE_AND_NULL(res);
      // return MEMORY_ERROR;
      // continue;
    }
    void *result_ptr = shmat(res_id, NULL, 0);
    if (result_ptr == (void *)-1) {
      fprintf(stderr, "shmat result");
      FREE_AND_NULL(res);
      continue;
    }
    *(size_t *)result_ptr = res_size;
    strcpy((char *)result_ptr + sizeof(size_t), res);
    FREE_AND_NULL(res);

    if (SEM_POST(sem, SERVER_RES_IX) != SUCCESS) {
      fprintf(stderr, "Ошибка SEM_POST SERVER_RES_IX\n");
      shmdt(result_ptr);
      continue;
    }
    if (SEM_WAIT(sem, CLIENT_RES_IX) != SUCCESS) {
      fprintf(stderr, "Ошибка SEM_WAIT CLIENT_RES_IX\n");
      shmdt(result_ptr);
      continue;
    }
    if (SEM_POST(sem, CLIENT_IX) != SUCCESS) {
      fprintf(stderr, "Ошибка SEM_POST CLIENT_IX\n");
      shmdt(result_ptr);
      continue;
    }
    printf("Сервер: обработка клиента завершена.\n");
    shmdt(result_ptr);
  }
  return 0;
}
