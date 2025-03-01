#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "shared.h"

#define MAX_SIZE (4096 * 10)

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

    DIR *dirp = NULL; // тек. директория
    struct dirent *dentry = NULL; // инфа о ней

    if ((dirp = opendir(result)) == NULL) {
        fprintf(stderr, "opendir ошибка\n");
        closedir(dirp);
        return ERROR_OPEN;
    }

    size_t cap = 30 + 14;
    char *BUF = malloc(30 + 7 + 7);
    if (BUF == NULL) {
        return MEMORY_ERROR;
    }
    size_t off = 0;
    off += sprintf(off + BUF, "Сейчас смотрим: %s\n", result);

    while ((dentry = readdir(dirp)) != NULL) {
        char name[PATH_MAX];
        snprintf(name, sizeof(name) / sizeof(char), "%s/%s", result, dentry->d_name);
        const size_t len = strlen(name) + 30 + 7 + 7 + off;
        if (len >= cap) {
            char *tmp = (char *) realloc(BUF, cap + (2 * len));
            if (tmp == NULL) {
                free(BUF);
                return MEMORY_ERROR;
            }
            BUF = tmp;
            cap += (2 * len);
        }
        // printf("%s\n", name);
        switch (dentry->d_type) {
            case DT_DIR: {
                off += sprintf(BUF + off, "директория: %s ino = %lu\n", name, dentry->d_ino);
            }
            break;
            case DT_REG: {
                off += sprintf(BUF + off, "файл: %s ino = %lu\n", name, dentry->d_ino);
            }
            break;
            case DT_LNK: {
                off += sprintf(BUF + off, "симв. ссылка: %s ino = %lu\n", name, dentry->d_ino);
            }
            break;
            case DT_UNKNOWN: {
                off += sprintf(BUF + off, "?: %s ino = %lu\n", name, dentry->d_ino);
            }
            break;
            case DT_BLK: {
                off += sprintf(BUF + off, "диск (блочное устройство): %s ino = %lu\n", name, dentry->d_ino);
            }
            break;
            case DT_CHR: {
                off += sprintf(BUF + off, "диск (символьное устройство): %s ino = %lu\n", name, dentry->d_ino);
            }
            break;
            case DT_FIFO: {
                off += sprintf(BUF + off, "именнованный канал: %s ino = %lu\n", name, dentry->d_ino);
            }
            break;
            case DT_SOCK: {
                off += sprintf(BUF + off, "сокет: %s ino = %lu\n", name, dentry->d_ino);
            }
            break;
            default: {
                fprintf(stderr, "err");
            }
            break;
        }
    }
    strcpy(*res, BUF);
    *res_size = strlen(BUF);
    closedir(dirp);
    free(BUF);
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
    if (size_id == -1) {
        semctl(size_id, 0, IPC_RMID);
        return MEMORY_ERROR;
    } {
        int existing_sem = semget(semkey, 2, 0666);
        if (existing_sem != -1) {
            semctl(existing_sem, 0, IPC_RMID);
        }
    }

    sem = semget(semkey, 2, IPC_CREAT | IPC_EXCL | 0666);
    if (sem == -1) {
        perror("semget");
        return SEM_ERR;
    } {
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

    // if ((shm = shmget(skey, MAX_SIZE, IPC_CREAT | 0666)) == -1) {
    //     perror("shmget data\n");
    //     return MEMORY_ERROR;
    // }
    //
    // if ((meow = shmat(shm, NULL, 0)) == (void *) -1) {
    //     printf("shmat");
    //     return MEMORY_ERROR;
    // }
    // CLEAR(meow, MAX_SIZE);

    printf("Сервер запустился!!!\n");

    while (1) {
        // printf("%d %d %d\n", sem, SERVER_ix, semctl(sem, SERVER_ix, GETVAL, 0));
        if (SEM_WAIT(sem, SERVER_ix) != SUCCESS) {
            fprintf(stderr, "Ошибка SEM_WAIT SERVER_ix\n");
            //   continue;
        }

        // тут живет размер строк, которых мы получили (т.е. тип размер shm client-а
        info = shmat(size_id, NULL, 0);
        if (info == (size_t *) -1) {
            perror("shmget info\n");
            return MEMORY_ERROR;
        }

        printf("%lu\n", info[0]);

        if ((shm = shmget(skey, *info, IPC_CREAT | 0666)) == -1) {
            perror("shmget data\n");
            return MEMORY_ERROR;
        }

        if ((meow = shmat(shm, NULL, 0)) == (void *) -1) {
            printf("shmat");
            return MEMORY_ERROR;
        }
        // CLEAR(meow, *info);

        const size_t *sPtr = (size_t *) meow;
        char *mPtr = (char *) meow + sizeof(size_t);

        if (*sPtr == 0) {
            printf("Сервер получил сигнал завершения. Выход...\n");
        } else {
            size_t off = 0;
            printf("Сервер начал обработку\n");

            printf("Путей получено = %lu\n", *sPtr);
            char **pre_buf = malloc(*sPtr * sizeof(char *));
            if (pre_buf == NULL) {
                return MEMORY_ERROR;
            }
            for (size_t i = 0; i < *sPtr; i++) {
                size_t len = 0;
                printf("path=%s\n", mPtr + off);
                dir_view(mPtr + off, pre_buf + i, &len);
                off += strlen(mPtr + off) + 1;
            }
            printf("%s", pre_buf[0]);


            free(pre_buf);
        }

        CLEAR(meow, MAX_SIZE);

        if (SEM_POST(sem, CLIENT_IX) != SUCCESS) {
            fprintf(stderr, "Ошибка SEM_POST CLIENT_IX\n");
        } else {
            printf("Сервер отработал клиента!\n");
        }

        if (shmdt(meow) == -1) {
            perror("shmdt\n");
            return MEMORY_ERROR;
        }
    }

    if (shmdt(meow) == -1) {
        printf("shmdt");
    }
    semctl(sem, 0, IPC_RMID);

    return 0;
}
