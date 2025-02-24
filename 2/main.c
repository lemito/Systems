#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "../include/base.h"

#define pow2(n) (1 << (n))

STATUS_CODE str_to_uint(const char *str, unsigned int *out, const int base) {
    if (str == NULL || *str == '\0' || out == NULL || base < 2 || base > 36) { return INPUT_ERROR; }
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

STATUS_CODE do_action(FILE *file, char **action, char **attr) {
    if (action == NULL || *action == NULL || attr == NULL || *attr == NULL || file == NULL) {
        return INPUT_ERROR;
    }

    if (strncmp(*action, "xor", 3) == 0) {
        const int N = *action[4] - '0';
        int mod = pow2(N);
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
    char *action = (strcpy(argv[argc - 1], "mask") == 0 || strcpy(argv[argc - 1], "find") == 0)
                       ? argv[argc - 2]
                       : argv[argc - 1];
    char *attr = argv[argc - 1];

    for (int i = 1; i < argc - 1; i++) {
        if (argv[i] == NULL) {
            return INPUT_ERROR;
        }
        FILE *file = fopen(argv[i], "r");
        if (file == NULL) {
            printf("Ошибка открытия файла");
            return INPUT_ERROR;
        }
        do_action(file, &action, &attr);
        fclose(file);
    }

    return 0;
}
