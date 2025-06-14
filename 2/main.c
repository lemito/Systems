#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../include/base.h"

#define MSG_SIZE 6144

typedef struct msg_text {
  int qid;
  char buf[MSG_SIZE];
} msg_text;

typedef struct msg {
  long mtype;
  msg_text data;
} msg;

typedef char byte;

char check_parallel(const char *action) {
  if (action == 0) {
    return 0;
  }

  if (strncmp(action, "copy", strlen("copy")) == 0 ||
      strncmp(action, "find", strlen("find")) == 0) {
    return 1;
  }

  return 0;
}

STATUS_CODE check_hex(const char *hex) {
  if (hex == NULL) {
    return INPUT_ERROR;
  }
  while (*hex) {
    if (!isxdigit(*hex)) {
      return ERROR_OPEN;
    }
    hex++;
  }
  return SUCCESS;
}

STATUS_CODE str_to_uint(const char *str, int *out, const int base) {
  if (str == NULL || *str == '\0' || out == NULL || base < 2 || base > 36) {
    return INPUT_ERROR;
  }
  char *end_ptr;
  *out = strtol(str, &end_ptr, base);
  if ((*out == LONG_MAX) || (*out == LONG_MIN)) {
    printf("str_to_int: выход за пределы long ");
    return INPUT_ERROR;
  }
  if (*end_ptr != '\0') {
    printf(
        "str_to_int: число должно содержать только циферки (и знак в начале) ");
    return INPUT_ERROR;
  }

  return SUCCESS;
}

STATUS_CODE xor2(FILE *file) {
  if (file == NULL) {
    return INPUT_ERROR;
  }
  byte res = 0x00;
  byte one = 0x00;
  while (fread(&one, sizeof(byte), 1, file)) {
    // байт == 8 бит; сдвигаемся на 4 чтобы получить верхнюю 4 битов
    const byte high_byte = one >> 4;
    // const byte high_bytes = one & 0xF0;
    // получаем последние 4 бита
    const byte low_byte = one & 0x0F;
    res ^= high_byte;
    res ^= low_byte;
  }
  printf("xor2: %x\n", res);
  return SUCCESS;
}

STATUS_CODE xor3(FILE *const file) {
  if (file == NULL) {
    return INPUT_ERROR;
  }
  byte res = 0x00;
  byte one = 0x00;
  while (fread(&one, 1, 1, file)) {
    res ^= one;
  }

  printf("xor3: %x\n", res);
  return SUCCESS;
}

STATUS_CODE xor4(FILE *file) {
  if (file == NULL) {
    return INPUT_ERROR;
  }
  // byte res[2] = {0x00, 0x00};
  union res_t {
    byte bytes[2];
    uint16_t num;
  };
  union res_t res;
  res.num = 0;
  byte one[2] = {0x00, 0x00};
  // группируем группу байт согласно размеру, в случае чего дозаполняем нулями
  size_t st = 0;
  while ((st = fread(&one, sizeof(one), 1, file)) != 0) {
    if (st < sizeof(one)) {
      for (size_t i = st; i < sizeof(one); i++) {
        one[i] = 0x00;
      }
    }
    for (size_t i = 0; i < sizeof(one); i++) {
      res.bytes[i] ^= one[i];
    }

    for (size_t i = 0; i < sizeof(one); i++) {
      one[i] = 0x00;
    }
  }
  printf("xor4:\n");
  for (size_t i = 0; i < sizeof(one); i++) {
    printf("%x ", res.bytes[i]);
  }
  printf("\nNum %d(10) %x(hex)", res.num, res.num);
  return SUCCESS;
}

STATUS_CODE xor5(FILE *file) {
  if (file == NULL) {
    return INPUT_ERROR;
  }
  // byte res[4] = {0x00, 0x00, 0x00, 0x00};
  union res_t {
    byte bytes[4];
    uint32_t num;
  };
  union res_t res;
  res.num = 0;
  byte one[4] = {0x00, 0x00, 0x00, 0x00};
  size_t st = 0;
  while ((st = fread(&one, sizeof(one), 1, file))) {
    if (st < sizeof(one)) {
      for (size_t i = st; i < 4; i++) {
        one[i] = 0x00;
      }
    }
    for (size_t i = 0; i < sizeof(one); i++) {
      res.bytes[i] ^= one[i];
    }
  }

  printf("xor5:\n");
  for (size_t i = 0; i < sizeof(one); i++) {
    printf("%x ", res.bytes[i]);
  }
  printf("\nNum %d(10) %x(hex)", res.num, res.num);
  return SUCCESS;
}

STATUS_CODE xor6(FILE *file) {
  if (file == NULL) {
    return INPUT_ERROR;
  }
  // byte res[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  union res_t {
    byte bytes[8];
    uint64_t num;
  };
  union res_t res;
  res.num = 0;
  byte one[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  size_t st = 0;
  while ((st = fread(&one, sizeof(one), 1, file))) {
    if (st < sizeof(one)) {
      for (size_t i = st; i < sizeof(one); i++) {
        one[i] = 0x00;
      }
    }
    for (size_t i = 0; i < sizeof(one); i++) {
      res.bytes[i] ^= one[i];
    }
  }

  printf("xor6:\n");
  for (size_t i = 0; i < sizeof(one); i++) {
    printf("%x ", res.bytes[i]);
  }
  printf("\nNum %ld(10) %llx(hex)", res.num, res.num);
  return SUCCESS;
}

// SUCESS - найдено; ERROR_OPEN - не найдено
STATUS_CODE find(FILE *file, const char **const attr) {
  if (file == NULL || attr == NULL || *attr == NULL) {
    return INPUT_ERROR;
  }

  const char *pattern = *attr;
  const size_t len = strlen(pattern);

  int ch = 0;
  long long ix = 0;

  while ((ch = fgetc(file)) != EOF) {
    if (ch == pattern[ix]) {
      ix++;
    } else {
      if (ix > 0 && ch == pattern[0]) {
        ix = 1;
      } else {
        fseek(file, -ix, SEEK_CUR);
        ix = 0;
      }
    }

    if (ix == len) {
      return SUCCESS;
    }
  }

  return ERROR_OPEN;
}

STATUS_CODE to_string(size_t src, char **res) {
  if (src == 0) {
    (*res) = (char *)malloc(2 * sizeof(char));
    if ((*res) == NULL) {
      return MEMORY_ERROR;
    }
    (*res)[0] = '0';
    (*res)[1] = '\0';
    return SUCCESS;
  }
  size_t len = (size_t)(log(src) / log(10)) + 1;
  *res = (char *)malloc((len + 1) * sizeof(char));
  if (*res == NULL) {
    return MEMORY_ERROR;
  }

  (*res)[len] = '\0';

  while (src > 0) {
    (*res)[--len] = src % 10 + '0';
    src /= 10;
  }
  return SUCCESS;
}

STATUS_CODE gen_new_name(char **filename, const size_t ix, char **res) {
  if (filename == NULL || res == NULL) {
    return INPUT_ERROR;
  }

  char *part2 = NULL;
  to_string(ix, &part2);

  const size_t len = strlen(*filename);
  const size_t len2 = strlen(part2);
  const size_t len3 = len + len2;
  *res = (char *)malloc((len3 + 1) * sizeof(char));
  if (*res == NULL) {
    free(part2);
    return MEMORY_ERROR;
  }
  (*res)[len3] = '\0';
  if (NULL == strcpy(*res, *filename)) {
    return MEMORY_ERROR;
  }
  if (NULL == strcat(*res, part2)) {
    return MEMORY_ERROR;
  }

  free(part2);

  return SUCCESS;
}

STATUS_CODE copy(FILE **file, const int i, const char **const filename) {
  if (file == NULL || filename == NULL) {
    return INPUT_ERROR;
  }

  FILE *filecpy = *file;

  for (size_t j = 0; j < i; j++) {
    const char *part1 = *filename;
    char *res = NULL;

    gen_new_name(&part1, j, &res);

    FILE *new_file = fopen(res, "wb");
    if (new_file == NULL) {
      return ERROR_OPEN;
    }

    char ch;
    while (fread(&ch, sizeof(char), 1, filecpy) != 0) {
      fwrite(&ch, sizeof(char), 1, new_file);
    }

    fclose(new_file);
    free(res);
    fseek(filecpy, 0, SEEK_SET);
  }

  return SUCCESS;
}

char *get_absolute_path(const char *path, char buf[FILENAME_MAX]) {
  char *tmp = realpath(path, buf);
  if (tmp == NULL) {
    printf("Ошибка получения абсолютного пути\n");
    return NULL;
  }
  return tmp;
}

/**
 * 100111 == 27(16) маска
 * 100111 == 47 - число
 * 111111 == 63 - число
 * @param mask
 * @param res
 * @param file
 * @return
 */
STATUS_CODE cnt_by_mask(const int *mask, size_t *res, FILE *file) {
  if (mask == NULL || res == NULL || file == NULL) {
    return INPUT_ERROR;
  }
  int obj = 0;
  while (fread(&obj, sizeof(int), 1, file) != 0) {
    // if ((obj & *mask) != 0) {
    if ((obj & *mask) == *mask) {
      (*res)++;
    }
  }
  fseek(file, 0, SEEK_SET);
  return SUCCESS;
}

/*
STATUS_CODE do_action(FILE **file, const char **const action,
                      const char **const attr, const char **const filename) {
  if (action == NULL || *action == NULL || attr == NULL || *attr == NULL ||
      file == NULL) {
    return INPUT_ERROR;
  }

  pid_t pid = 0;
  int status = 0;

  if (strncmp(*action, "xor", 3) == 0) {
    const int N = (*action)[3] - '0';
    switch (N) {
      case 2: {
        xor2(*file);
      } break;
      case 3: {
        xor3(*file);
      } break;
      case 4: {
        xor4(*file);
      } break;
      case 5: {
        xor5(*file);
      } break;
      case 6: {
        xor6(*file);
      } break;
      default: {
        printf("Упс... N = [2...6]\n");
      } break;
    }
  } else if (strncmp(*action, "copy", 4) == 0) {
    const size_t len = strlen(*action);
    int N = 0;  // количество копий файлов
    size_t pow = 1;
    for (size_t i = 4; i < len; i++) {
      N += ((*action)[i] - '0') * 1;
      pow *= 10;
    }
    pid = fork();
    switch (pid) {
      case 0: {
        if (SUCCESS != copy(file, N, filename)) {
          printf("mew");
          // return ERROR_OPEN;
          exit(ERROR_OPEN);
        }
        // return SUCCESS;
        exit(SUCCESS);
      }
      case -1:
        // return FORK_ERROR;
      default: {
        wait(&status);  // ждемс чилда, пока он там все поищет
      }
    }
  } else if (strncmp(*action, "mask", 4) == 0) {
    int mask = 0;
    if (ERROR_OPEN == check_hex(*attr)) {
      return INPUT_ERROR;
    }
    if (SUCCESS != str_to_uint(*attr, &mask, 16)) {
      return INPUT_ERROR;
    }
    size_t res = 0;
    if (INPUT_ERROR == cnt_by_mask(&mask, &res, *file)) {
      return INPUT_ERROR;
    }
    printf("Четырехбайтовых чисел, удовлетворяющих маске %s найдено %ld\n",
           *attr, res);
  } else if (strncmp(*action, "find", 4) == 0) {
    pid = fork();
    switch (pid) {
      case 0: {
        char BUF[FILENAME_MAX];
        if (NULL == get_absolute_path(*filename, BUF)) {
          // return MEMORY_ERROR;
          exit(MEMORY_ERROR);
        }
        if (SUCCESS != find(*file, attr)) {
          printf("%s не нашлось в %s :(", *attr, BUF);
          // return NULL_PTR;
          exit(NULL_PTR);
        }
        printf("%s был найден в %s\n", *attr, BUF);
        // return SUCCESS;
        exit(SUCCESS);
      }
      case -1:
        // return FORK_ERROR;
        exit(FORK_ERROR);
      default: {
        wait(&status);  // ждемс чилда, пока он там все поищет
        // printf("st=%d\n",status);
        switch (status) {
          case MEMORY_ERROR: {
            return MEMORY_ERROR;
          }
            // case SUCCESS: {printf("%s был найден в %s\n", *attr, BUF);}
            // break; case NULL_PTR: {printf("%s не нашлось в %s :(", *attr,
            // BUF);}
        }
      }
    }
  } else {
    return INPUT_ERROR;
  }

  return SUCCESS;
}
*/

STATUS_CODE xor_action(char **files, int size, const int N) {
  if (files == NULL || N < 2 || N > 7) {
    return INPUT_ERROR;
  }
  size -= 1;
  for (size_t i = 0; i < size; i++) {
    FILE *fp = NULL;
    fp = fopen(files[i], "rb");
    if (fp == NULL) {
      printf("ошибка файла %s\n", files[i]);

      return ERROR_OPEN;
    }
    printf("Результат для файла %s: \n", files[i]);
    switch (N) {
      case 2: {
        xor2(fp);
      } break;
      case 3: {
        xor3(fp);
      } break;
      case 4: {
        xor4(fp);
      } break;
      case 5: {
        xor5(fp);
      } break;
      case 6: {
        xor6(fp);
      } break;
      default: {
        printf("Упс... N = [2...6]\n");
      } break;
    }
    FCLOSE(fp);
    printf("====\n");
  }
  return SUCCESS;
}

STATUS_CODE copy_action(char **files, int size, const int N) {
  if (files == NULL || N < 2 || N > 7) {
    return INPUT_ERROR;
  }
  size -= 1;
  for (size_t i = 0; i < size; i++) {
    FILE *fp = NULL;
    fp = fopen(files[i], "rb");
    if (fp == NULL) {
      printf("ошибка файла %s\n", files[i]);

      return ERROR_OPEN;
    }

    const pid_t pid = fork();
    switch (pid) {
      case -1: {
        perror("fork");
        continue;
      }
      case 0: {
        if (SUCCESS != copy(&fp, N, files + i)) {
          printf("mew");
          exit(ERROR_OPEN);
        }
        exit(SUCCESS);
      }
      default: {
      } break;
    }

    FCLOSE(fp);
  }
  return SUCCESS;
}

STATUS_CODE mask_action(char **files, int size, const char *attr) {
  if (files == NULL || attr == NULL) {
    return INPUT_ERROR;
  }
  size -= 1;
  for (size_t i = 0; i < size; i++) {
    FILE *fp = NULL;
    fp = fopen(files[i], "rb");
    if (fp == NULL) {
      printf("ошибка файла %s\n", files[i]);

      return ERROR_OPEN;
    }
    int mask = 0;
    if (ERROR_OPEN == check_hex(attr)) {
      return INPUT_ERROR;
    }
    if (SUCCESS != str_to_uint(attr, &mask, 16)) {
      return INPUT_ERROR;
    }
    size_t res = 0;
    if (INPUT_ERROR == cnt_by_mask(&mask, &res, fp)) {
      return INPUT_ERROR;
    }
    printf(
        "Четырехбайтовых чисел, удовлетворяющих маске %s найдено %ld в файле "
        "%s\n",
        attr, res, files[i]);
    FCLOSE(fp);
  }
  return SUCCESS;
}

typedef struct data {
  pid_t pid;
  const char *filename;
} data_t;

STATUS_CODE find_action(char **files, int size, const char *attr) {
  if (files == NULL || attr == NULL) {
    return INPUT_ERROR;
  }

  const int msgqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
  if (msgqid == -1) {
    perror("msgget");
    return INPUT_ERROR;
  }
  size -= 1;
  data_t *child_info = malloc(size * sizeof(data_t));
  if (NULL == child_info) {
    return MEMORY_ERROR;
  }
  size_t child_cnt = 0;

  for (size_t i = 0; i < size; i++) {
    FILE *fp = NULL;
    fp = fopen(files[i], "rb");
    if (fp == NULL) {
      FREE_AND_NULL(child_info);
      const int st = msgctl(msgqid, IPC_RMID, NULL);
      if (st == -1) {
        return SEM_ERR;
      }
      printf("ошибка файла %s\n", files[i]);
      return ERROR_OPEN;
    }

    const pid_t pid = fork();
    switch (pid) {
      case -1: {
        perror("fork");
        continue;
      }
      case 0: {
        char BUF[FILENAME_MAX];
        msg res = {0};

        if (NULL == get_absolute_path(files[i], BUF)) {
          exit(MEMORY_ERROR);
        }
        const int st = find(fp, &attr);
        res.mtype = getpid();
        res.data.qid = getpid();
        // printf("%d\n", msgqid);
        if (st == SUCCESS) {
          snprintf(res.data.buf, MSG_SIZE, "%s был найден в %s\0", attr, BUF);
        } else if (st == ERROR_OPEN) {
          snprintf(res.data.buf, MSG_SIZE, "%s не был найден в %s\0", attr,
                   BUF);
        }
        if (msgsnd(msgqid, &res, sizeof(res.data), 0) == -1) {
          perror("msgsnd");
          exit(INPUT_ERROR);
        }
        exit(SUCCESS);
      }
      default: {
        child_info[child_cnt].pid = pid;
        child_info[child_cnt].filename = files[i];
        child_cnt++;
      } break;
    }

    FCLOSE(fp);
  }

  if (child_cnt == 0) {
    fprintf(stderr, "Не удалось обработать ни одного файла\n");
    FREE_AND_NULL(child_info);
    msgctl(msgqid, IPC_RMID, NULL);
    return ERROR_OPEN;
  }

  for (int i = 0; i < child_cnt; i++) {
    waitpid(child_info[i].pid, NULL, 0);
    msg res;
    if (msgrcv(msgqid, &res, sizeof(res.data), child_info[i].pid, 0) == -1) {
      perror("msgrcv");
      continue;
    }
    printf("%s\n", res.data.buf);
  }

  FREE_AND_NULL(child_info);

  const int st = msgctl(msgqid, IPC_RMID, NULL);
  if (st == -1) {
    return SEM_ERR;
  }
  return SUCCESS;
}

STATUS_CODE work_with_files(char **argv, const int argc) {
  if (argv == NULL) {
    return NULL_PTR;
  }

  const char *action = NULL;  // действие
  const char *attr = NULL;    // параметр
  int size = 0;

  action = (strcmp(argv[argc - 2], "mask") == 0 ||
            strcmp(argv[argc - 2], "find") == 0)
               ? argv[argc - 2]
               : argv[argc - 1];
  attr = argv[argc - 1];
  size = (strcmp(argv[argc - 2], "mask") == 0 ||
          strcmp(argv[argc - 2], "find") == 0)
             ? argc - 2
             : argc - 1;

  if (action == NULL) {
    return MEMORY_ERROR;
  }
  if (strncmp(action, "xor", 3) == 0) {
    const int N = action[3] - '0';
    return xor_action(argv + 1, size, N);
  }
  if (strncmp(action, "copy", 4) == 0) {
    const size_t len = strlen(action);
    int N = 0;  // количество копий файлов
    size_t pow = 1;
    for (size_t i = 4; i < len; i++) {
      N += (action[i] - '0') * pow;
      pow *= 10;
    }
    return copy_action(argv + 1, size, N);
  }
  if (strncmp(action, "mask", 4) == 0) {
    return mask_action(argv + 1, size, attr);
  }
  if (strncmp(action, "find", 4) == 0) {
    return find_action(argv + 1, size, attr);
  }
  return INPUT_ERROR;
}
//
// Created by lemito on 2/23/25.
//
int main(const int argc, char *argv[]) {
  if (argc < 3) {
    printf("Используй: %s <file> <file> <флаг>\n", argv[0]);
    return INPUT_ERROR;
  }

  const STATUS_CODE st = work_with_files(argv, argc);
  if (st != SUCCESS) {
    return st;
  }

  // for (int i = 1; i < size; i++) {
  //   if (argv[i] == NULL) {
  //     return INPUT_ERROR;
  //   }
  //
  //   FILE *file = fopen(argv[i], "rb");
  //   if (file == NULL) {
  //     printf("Ошибка открытия файла");
  //     return INPUT_ERROR;
  //   }
  //   const STATUS_CODE st = do_action(&file, &action, &attr, &argv[i]);
  //   if (st != SUCCESS) {
  //     FCLOSE(file);
  //     return st;
  //   }
  //
  //   FCLOSE(file);
  // }

  return 0;
}
