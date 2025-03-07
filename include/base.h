//
// Created by lemito on 2/23/25.
//

#ifndef BASE_H
#define BASE_H

enum STATUS_CODE {
  SUCCESS = 0,
  INPUT_ERROR,
  NULL_PTR,
  ERROR_OPEN,
  MEMORY_ERROR,
  USER_NOT_CONTAIN,
  USER_IS_CONTAIN,
  SEM_ERR,
  FORK_ERROR,
  THREAD_ERROR
};

typedef enum STATUS_CODE STATUS_CODE;

union semun {
  int val;               /* значение для SETVAL */
  struct semid_ds *buf;  /* буфер для IPC_STAT, IPC_SET */
  unsigned short *array; /* массив для GETALL, SETALL */
  struct seminfo *__buf; /* буфер для IPC_INFO (Linux-specific) */
};

#define CLEAR_BUF()           \
  while (getchar() != '\n') { \
  }

#define FREE_AND_NULL(ptr) \
  if (ptr != NULL) {       \
    free(ptr);             \
    ptr = NULL;            \
  }
#define FCLOSE(fp)  \
  if (fp != NULL) { \
    fclose(fp);     \
  }

#endif  // BASE_H
