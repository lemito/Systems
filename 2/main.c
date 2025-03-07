#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../include/base.h"

#define pow2(n) (1 << (n))
typedef char byte;

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
    for (size_t i = 0; i < 4; i++) {
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
    for (size_t i = 0; i < 4; i++) {
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
          return ERROR_OPEN;
        }
        return SUCCESS;
      }
      case -1:
        return FORK_ERROR;
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
          return MEMORY_ERROR;
        }
        if (SUCCESS != find(*file, attr)) {
          printf("%s не нашлось в %s :(", *attr, BUF);
          return NULL_PTR;
        }
        printf("%s был найден в %s\n", *attr, BUF);
        return SUCCESS;
      }
      case -1:
        return FORK_ERROR;
      default: {
        wait(&status);  // ждемс чилда, пока он там все поищет
        // printf("st=%d\n",status);
        switch (status) {
          case MEMORY_ERROR: {return MEMORY_ERROR;}
          // case SUCCESS: {printf("%s был найден в %s\n", *attr, BUF);} break;
          // case NULL_PTR: {printf("%s не нашлось в %s :(", *attr, BUF);}
        }
      }
    }
  } else {
    return INPUT_ERROR;
  }

  return SUCCESS;
}

//
// Created by lemito on 2/23/25.
//
int main(const int argc, char *argv[]) {
  if (argc < 3) {
    printf("Используй: %s <file> <file> <флаг>\n", argv[0]);
    return INPUT_ERROR;
  }

  const char *action = NULL;
  const char *attr = NULL;
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

  for (int i = 1; i < size; i++) {
    if (argv[i] == NULL) {
      return INPUT_ERROR;
    }
    FILE *file = fopen(argv[i], "rb");
    if (file == NULL) {
      printf("Ошибка открытия файла");
      return INPUT_ERROR;
    }
    STATUS_CODE st = do_action(&file, &action, &attr, &argv[i]);
    if (st != SUCCESS) {
      fclose(file);
      return st;
    }
    fclose(file);
  }

  return 0;
}
