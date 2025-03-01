#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

#include "shared.h"

typedef struct data {
  void *mem;
  size_t str_cnt;
} data_t;

int main(const int argc, char **argv) {
  if (argc < 2) {
    printf("Используй %s <abs path0> <abs path1> <abs path2> <abs path3> ...\n",
           argv[0]);
    return INPUT_ERROR;
  }
  key_t skey, semkey, infokey;
  int shm;
  int sem;
  int size_id;
  void *meow = NULL;
  char *ptr = NULL;
  size_t offset = 0;
  size_t to_sharing = sizeof(size_t);

  for (size_t i = 1; i < argc; i++) {
    if (argv[i] == NULL) {
      return INPUT_ERROR;
    }
    to_sharing += (strlen(argv[i]) + 1) * sizeof(char);
  }
  // if ((infokey = ftok(INFO_NAME, 'S')) == -1) {
  //   perror("ftok sizekey\n");
  // }
  // size_id = shmget(infokey, sizeof(info_t), 0666);
  // if (size_id == -1) {
  //   // semctl(size_id, 0, IPC_RMID);
  //   perror("shmget sizeid");
  //   return MEMORY_ERROR;
  // }
  //
  // info_t *sizee = shmat(size_id, NULL, 0);
  // if (sizee == -1) {
  //   perror("shmat sizeid");
  //   return MEMORY_ERROR;
  // }
  //
  // sizee->data_size = to_sharing;

  if ((skey = ftok(SHM_NAME, 'S')) == -1) {
    perror("ftok shm\n");
    return INPUT_ERROR;
  }
  if ((semkey = ftok(SEM_NAME, 'b')) == -1) {
    perror("ftok sem\n");
    return INPUT_ERROR;
  }
  if ((shm = shmget(skey, to_sharing, 0666)) == -1) {
    perror("shmget\n");
    return MEMORY_ERROR;
  }
  // sizee->shm_id = shmget(IPC_PRIVATE, to_sharing, IPC_CREAT | 0666);
  // if (sizee->shm_id == -1) {
  //   perror("shmget data");
  //   return MEMORY_ERROR;
  // }
  if ((sem = semget(semkey, 2, 0666)) == -1) {
    perror("semget\n");
    return SEM_ERR;
  }

  if ((meow = shmat(shm, NULL, 0)) == (void *)-1) {
    printf("shmat");
    return MEMORY_ERROR;
  }
  if (meow == NULL) {
    return MEMORY_ERROR;
  }

  CLEAR(meow, to_sharing);

  size_t *tmp = meow;
  *tmp = argc - 1;
  ptr = (char *)meow + sizeof(size_t);

  for (size_t i = 1; i < argc; i++) {
    printf("%s отправлено\n", argv[i]);
    strcpy(ptr + offset, argv[i]);
    offset += strlen(argv[i]) + 1;
  }

  if (SEM_POST(sem, SERVER_ix) != SUCCESS) {
    fprintf(stderr, "Ошибка SEM_POST SERVER_ix\n");
  }
  printf("Вы свободны!\n");
  if (SEM_WAIT(sem, CLIENT_IX) != SUCCESS) {
    fprintf(stderr, "Ошибка SEM_WAIT CLIENT_IX\n");
  }

  if (shmdt(meow) == -1) {
    printf("shmdt");
  }

  return 0;
}
