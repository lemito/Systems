#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/base.h"

#define pow2(n) (1 << (n))
typedef char byte;

STATUS_CODE str_to_uint(const char *str, unsigned int *out, const int base) {
    if (str == NULL || *str == '\0' || out == NULL || base < 2 || base > 36) {
        return INPUT_ERROR;
    }
    char *end_ptr;
    *out = strtoul(str, &end_ptr, base);
    if ((*out == ULONG_MAX)) {
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
    byte res[2] = {0x00, 0x00};
    byte one[2] = {0x00, 0x00};
    size_t st = 0;
    while ((st = fread(&one, sizeof(one), 1, file)) != 0) {
        if (st != sizeof(one)) {
            for (size_t i = st; i < sizeof(one); i++) {
                res[i] = 0x00;
            }
        }
        for (size_t i = 0; i < sizeof(one); i++) {
            res[i] ^= one[i];
        }
    }
    printf("xor4:\n");
    for (size_t i = 0; i < sizeof(one); i++) {
        printf("%x ", res[i]);
    }
    return SUCCESS;
}

STATUS_CODE xor5(FILE *file) {
    if (file == NULL) {
        return INPUT_ERROR;
    }
    byte res[4] = {0x00, 0x00, 0x00, 0x00};
    byte one[4] = {0x00, 0x00, 0x00, 0x00};
    size_t st = 0;
    while ((st = fread(&one, sizeof(one), 1, file))) {
        if (st != 4) {
            for (size_t i = st; i < 4; i++) {
                one[i] = 0x00;
            }
        }
        for (size_t i = 0; i < 4; i++) {
            res[i] ^= one[i];
        }
    }

    printf("xor5:\n");
    for (size_t i = 0; i < sizeof(one); i++) {
        printf("%x ", res[i]);
    }
    return SUCCESS;
}

STATUS_CODE xor6(FILE *file) {
    if (file == NULL) {
        return INPUT_ERROR;
    }
    byte res[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    byte one[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t st = 0;
    while ((st = fread(&one, sizeof(one), 1, file))) {
        if (st != 4) {
            for (size_t i = st; i < sizeof(one); i++) {
                one[i] = 0x00;
            }
        }
        for (size_t i = 0; i < 4; i++) {
            res[i] ^= one[i];
        }
    }

    printf("xor5:\n");
    for (size_t i = 0; i < sizeof(one); i++) {
        printf("%x ", res[i]);
    }
    return SUCCESS;
}

STATUS_CODE do_action(FILE *file, char **action, char **attr) {
    if (action == NULL || *action == NULL || attr == NULL || *attr == NULL ||
        file == NULL) {
        return INPUT_ERROR;
    }

    if (strncmp(*action, "xor", 3) == 0) {
        const int N = (*action)[3] - '0';
        switch (N) {
            case 2: {
                if (SUCCESS != xor2(file)) {
                    return INPUT_ERROR;
                }
            }
            break;
            case 3: {
                if (SUCCESS != xor3(file)) {
                    return INPUT_ERROR;
                }
            }
            break;
            case 4: {
                if (SUCCESS != xor4(file)) {
                    return INPUT_ERROR;
                }
            }
            break;
            case 5: {
                if (SUCCESS != xor5(file)) {
                    return INPUT_ERROR;
                }
            }
            break;
            case 6: {
                if (SUCCESS != xor6(file)) {
                    return INPUT_ERROR;
                }
            }
            break;
            default: {
                printf("Упс... N = [2...6]\n");
            }
            break;
        }
    } else if (strncmp(*action, "copy", 4) == 0) {
        const size_t len = strlen(*action);
        int N = 0; // количество копий файлов
        for (size_t i = 4; i < len; i++) {
            N += (*action[i] - '0') * 10;
        }
    } else if (strncmp(*action, "mask", 4) == 0) {
        unsigned int mask = 0;
        if (SUCCESS != str_to_uint(*attr, &mask, 16)) {
            return INPUT_ERROR;
        }
    } else if (strncmp(*action, "find", 4) == 0) {
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

    char *action = NULL;
    char *attr = NULL;

    action = (strcmp(argv[argc - 1], "mask") == 0 ||
              strcmp(argv[argc - 1], "find") == 0)
                 ? argv[argc - 2]
                 : argv[argc - 1];
    attr = argv[argc - 1];

    for (int i = 1; i < argc - 1; i++) {
        if (argv[i] == NULL) {
            return INPUT_ERROR;
        }
        FILE *file = fopen(argv[i], "rb");
        if (file == NULL) {
            printf("Ошибка открытия файла");
            return INPUT_ERROR;
        }
        do_action(file, &action, &attr);
        fclose(file);
    }

    return 0;
}
