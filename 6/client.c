#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

#include "shared.h"

int main(const int argc, char **argv) {
  if (argc < 2) {
    printf("Используй: %s <abs path0> <abs path1> ...\n", argv[0]);
    return INPUT_ERROR;
  }
  key_t skey, semkey, infokey, reskey;
  void *meow = NULL;
  char *ptr = NULL;
  size_t *sizee = NULL;
  size_t offset = 0;
  size_t to_sharing = sizeof(size_t);

  for (size_t i = 1; i < (size_t)argc; i++) {
    if (argv[i] == NULL) return INPUT_ERROR;
    to_sharing += (strlen(argv[i]) + 1) * sizeof(char);
  }

  if ((infokey = ftok(INFO_NAME, 'S')) == -1) {
    fprintf(stderr, "ftok sizekey");
    return INPUT_ERROR;
  }
  const int size_id = shmget(infokey, sizeof(size_t), 0666);
  if (size_id == -1) {
    fprintf(stderr, "shmget sizeid");
    return MEMORY_ERROR;
  }
  sizee = shmat(size_id, NULL, 0);
  if (sizee == (void *)-1) {
    fprintf(stderr, "shmat sizeid");
    return MEMORY_ERROR;
  }
  *sizee = to_sharing;
  printf("Клиент: точный размер входных данных = %lu байт\n", *sizee);

  if ((skey = ftok(SHM_NAME, 'p')) == -1) {
    fprintf(stderr, "ftok shm");
    return (INPUT_ERROR);
  }
  if ((semkey = ftok(SEM_NAME, 'W')) == -1) {
    fprintf(stderr, "ftok sem");
    return (INPUT_ERROR);
  }
  if ((reskey = ftok(SHM_RESULT_NAME, 'p')) == -1) {
    fprintf(stderr, "ftok reskey");
    return (INPUT_ERROR);
  }

  const int sem = semget(semkey, 6, 0666);
  if (sem == -1) {
    fprintf(stderr, "semget");
    return (SEM_ERR);
  }

  if (SEM_POST(sem, SIZE_SERVER_ix) != SUCCESS) {
    fprintf(stderr, "Ошибка SEM_POST SIZE_SERVER_ix\n");
    return (SEM_ERR);
  }
  if (SEM_WAIT(sem, SIZE_CLIENT_ix) != SUCCESS) {
    fprintf(stderr, "Ошибка SEM_WAIT SIZE_CLIENT_ix\n");
    return (SEM_ERR);
  }

  const int cleaner = shmget(skey, to_sharing, 0666);
  if (cleaner != -1) {
    shmctl(cleaner, IPC_RMID, NULL);
  }
  const int shm = shmget(skey, to_sharing, IPC_CREAT | IPC_EXCL | 0666);
  if (shm == -1) {
    fprintf(stderr, "shmget data");
    return (MEMORY_ERROR);
  }
  meow = shmat(shm, NULL, 0);
  if (meow == (void *)-1) {
    fprintf(stderr, "shmat data");
    return (MEMORY_ERROR);
  }
  CLEAR(meow, to_sharing);
  *(size_t *)meow = argc - 1;
  ptr = (char *)meow + sizeof(size_t);
  for (size_t i = 1; i < (size_t)argc; i++) {
    printf("%s отправлено\n", argv[i]);
    strcpy(ptr + offset, argv[i]);
    offset += strlen(argv[i]) + 1;
  }
  if (SEM_POST(sem, SERVER_ix) != SUCCESS) {
    fprintf(stderr, "Ошибка SEM_POST SERVER_ix\n");
    return (SEM_ERR);
  }

  if (SEM_WAIT(sem, SERVER_RES_IX) != SUCCESS) {
    fprintf(stderr, "Ошибка SEM_WAIT SERVER_RES_IX\n");
    return (SEM_ERR);
  }

  const int shm_res_id = shmget(reskey, 1, 0666);
  if (shm_res_id == -1) {
    fprintf(stderr, "shmget result");
    return (MEMORY_ERROR);
  }
  char *res_ptr = shmat(shm_res_id, NULL, 0);
  if (res_ptr == (char *)-1) {
    fprintf(stderr, "shmat result");
    return (MEMORY_ERROR);
  }
  const size_t res_size = *(size_t *)res_ptr;
  char *res_str = (char *)res_ptr + sizeof(size_t);
  printf("Клиент: результат работы сервера (%lu байт):\n%s\n", res_size,
         res_str);
  if (SEM_POST(sem, CLIENT_RES_IX) != SUCCESS) {
    fprintf(stderr, "Ошибка SEM_POST CLIENT_RES_IX\n");
    return (SEM_ERR);
  }

  if (SEM_WAIT(sem, CLIENT_IX) != SUCCESS) {
    fprintf(stderr, "Ошибка SEM_WAIT CLIENT_IX\n");
    return (SEM_ERR);
  }
  shmdt(meow);
  shmdt(res_ptr);
  shmdt(sizee);
  shmctl(shm_res_id, IPC_RMID, NULL);

  return 0;
}
