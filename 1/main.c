//
// Created by lemito on 2/23/25.
//
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

#include "../include//base.h"

struct user_info {
    char name[7];
    int PIN; // unsigned
    ssize_t limit; // -1 - лимита нет, иначе число
    size_t cmd_cnt;
};

typedef struct user_info user_t;

struct db_t {
    user_t **data;
    size_t size;
};

typedef struct db_t db_t;

void help(void) {
    puts("0) Exit - завершить прогу");
    puts("1) Time - запрос текущего времени в стандартном формате чч:мм:сс");
    puts("2) Date - запрос текущей даты в стандартном формате дд:мм:гггг");
    puts(
        "3) Howmuch <time> flag - запрос прошедшего времени с указанной даты в параметре <time>, параметр flag определяет тип представления результата (-s в секундах, -m в минутах, -h в часах, -y в годах)");
    puts("4) Logout - выйти в меню авторизации");
    puts(
        "5) Sanctions username<number> - команда позволяет ввести ограничения  на работу с оболочкой для пользователя username а именно данный пользователь не может в одном сеансе выполнить более<number> запросов.Для подтверждения ограничений после ввода команды необходимо ввести значение 12345.");
}

// валидация пина
int check_pin(const int pin) {
    return ((pin >= 0) && (pin <= 100000));
}

int check_login(const char *login) {
    while (*login != '\0') {
        if (!isalnum(*login)) {
            return 0;
        }
        login++;
    }
    return 1;
}

user_t *user_is_contain(const db_t *db, const char *login) {
    for (int i = 0; i < db->size; i++) {
        if (strcmp(login, db->data[i]->name) == 0) {
            return db->data[i];
        }
    }
    return NULL;
}

STATUS_CODE sign_in_up(db_t *const db, user_t **res_user) {
    if (db == NULL) {
        return NULL_PTR;
    }
    char mode = 0; // 0 - вхлд; 1 - рега
    int iput_pin = 0;
    char buffer[27];
    int st = 0;
    user_t *user = NULL;

    printf("Необходимо войти или зарегистрироваться:\nLogin - Вход\nRegister - Регистрация\n");
    for (;;) {
        st = scanf("%26s", buffer);
        CLEAR_BUF();

        if (st == -1) {
            printf("Ошибка ввода\n");
            continue;
        }

        if (strcmp(buffer, "Login") == 0 || strcmp(buffer, "Register") == 0) {
            break;
        }

        printf("Ошибка ввода.\n");
        printf("Необходимо войти или зарегистрироваться:\nLogin - Вход\nRegister - Регистрация\n");
    }

    if (strcmp(buffer, "Login") == 0) { mode = '0'; } else if (strcmp(buffer, "Register") == 0) { mode = '1'; }
    buffer[0] = '\0';

    switch (mode) {
        // вход
        case '0': {
            for (;;) {
                printf("Введи имя пользователя(login): ");

                st = scanf("%s", buffer);
                CLEAR_BUF();
                if (st == -1) {
                    printf("Ошибка!!!!\n");
                    continue;
                }
                if (strlen(buffer) > 7) {
                    printf("Упсик... Ток 6 символов можно :(\n");
                    continue;
                }
                if (0 == check_login(buffer)) {
                    printf("Ошибка!!!! Используй латиницу и циферки\n");
                    continue;
                }

                if ((user = user_is_contain(db, buffer)) == NULL) {
                    printf("Такого пользователя нет! Советую зарегаться\n");
                    return USER_NOT_CONTAIN;
                    break;
                }
                printf("Мяу, %s!\n", buffer);
                break;
            }
            for (;;) {
                printf("Введите пароль: ");
                st = scanf("%d", &iput_pin);
                CLEAR_BUF();
                if (st == -1) {
                    printf("Ошибка!!!!\n");
                    continue;
                }
                if (check_pin(iput_pin) == 0) {
                    printf("Ошибка!!!! Используй пин из [0, 100000]\n");
                    continue;
                }

                if (user->PIN != iput_pin) {
                    printf("Упсик, неправильный логин\n");
                    continue;
                }

                printf("Добро пожаловать");
                break;
            }
            if (user->limit != -1) {
                printf("Ой-ой. У вас есть лимит по командам = %ld", user->limit);
            }
            *res_user = user;
        }
        break;
        // рега
        case '1': {
            for (;;) {
                printf("Введи имя пользователя(login): ");

                st = scanf("%s", buffer);
                CLEAR_BUF();
                if (st == -1) {
                    printf("Ошибка!!!!\n");
                    continue;
                }
                if (strlen(buffer) > 7) {
                    printf("Упсик... Ток 6 символов можно :(\n");
                    continue;
                }
                if (0 == check_login(buffer)) {
                    printf("Ошибка!!!! Используй латиницу и циферки\n");
                    continue;
                }

                if ((user = user_is_contain(db, buffer)) != NULL) {
                    printf("Пользователь с логином %s уже есть, попробуй другой логин\n", user->name);
                    return USER_IS_CONTAIN;
                    break;
                }
                printf("Мяу, %s!\n", buffer);
                break;
            }
            for (;;) {
                printf("Введите пароль: ");
                st = scanf("%d", &iput_pin);
                CLEAR_BUF();
                if (st == -1) {
                    printf("Ошибка!!!!\n");
                    continue;
                }
                if (check_pin(iput_pin) == 0) {
                    printf("Ошибка!!!! Используй пин из [0, 100000]\n");
                    continue;
                }

                break;
            }
            user_t *new_user = (user_t *) malloc(sizeof(user_t));
            new_user->limit = -1;
            new_user->cmd_cnt = 0;
            new_user->PIN = iput_pin;
            strcpy(new_user->name, buffer);
            user_t **tmp = realloc(db->data, sizeof(user_t *) * (db->size + 1));
            if (tmp == NULL) {
                return MEMORY_ERROR;
            }
            db->data = tmp;
            db->data[db->size++] = new_user;
            printf("Ураааааа! Счастливого пользования!\n");
            *res_user = new_user;
        }
        break;
        default: {
        }
        break;
    }

    return SUCCESS;
}

STATUS_CODE get_time(void) {
    const time_t t = time(NULL); // UNIX времечко
    if (t == -1) {
        return INPUT_ERROR;
    }
    const struct tm tm = *localtime(&t); // времечко UNIX -> структура
    printf("%02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
    return SUCCESS;
}

STATUS_CODE get_date(void) {
    const time_t t = time(NULL); // UNIX времечко
    if (t == -1) {
        return INPUT_ERROR;
    }
    const struct tm tm = *localtime(&t); // времечко UNIX -> структура
    printf("%02d:%02d:%d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
    return SUCCESS;
}

STATUS_CODE howmuch(void) {
    return SUCCESS;
}

STATUS_CODE upload_db(db_t *db, FILE *file) {
    if (db == NULL || file == NULL) {
        return NULL_PTR;
    }

    db->data = NULL;
    db->size = 0;

    while (1) {
        user_t *user = malloc(sizeof(user_t));
        if (user == NULL) {
            return MEMORY_ERROR;
        }

        size_t st = fread(user, sizeof(user_t), 1, file);
        if (st != 1) {
            free(user);
            if (feof(file)) {
                break;
            }
            return INPUT_ERROR;
        }

        user_t **tmp = realloc(db->data, (db->size + 1) * sizeof(user_t *));
        if (!tmp) {
            free(user);
            return MEMORY_ERROR;
        }

        db->data = tmp;
        db->data[db->size++] = user;
    }

    return SUCCESS;
}

STATUS_CODE save_db(const db_t *db, FILE *file) {
    if (db == NULL || file == NULL) {
        return NULL_PTR;
    }


    for (size_t i = 0; i < db->size; i++) {
        user_t *tm = db->data[i];
        fwrite(db->data[i], sizeof(user_t), 1, file);
    }

    return SUCCESS;
}

int main(void) {
    FILE *db_file = NULL;
    db_t db;
    char usage = 1;
    int st = 0;
    char buffer[BUFSIZ];
    user_t *user = NULL;

    db_file = fopen("users.db", "rb");
    if (db_file == NULL) {
        fprintf(stderr, "ошибка открытия файла базы\n");
        return ERROR_OPEN;
    }

    db.data = malloc(sizeof(user_t));
    if (db.data == NULL) {
        fclose(db_file);
        return MEMORY_ERROR;
    }
    db.size = 0;

    upload_db(&db, db_file);
    fclose(db_file);

    /* приветствие: входрега */
    if (sign_in_up(&db, &user) != SUCCESS) {
        free(db.data);
        fclose(db_file);
        return 1;
    }

    if (user == NULL) {
        free(db.data);
        fclose(db_file);
        return NULL_PTR;
    }

    /* actions */
    while (usage) {
        help();
        printf("Выбери действие: \n>> ");
        st = scanf("%s", buffer);
        CLEAR_BUF();
        if (st == EOF) {
            usage = 0;
            printf("Пока-пока\n");
        }

        if (strcmp("\0", buffer) == 0) {
            printf("Пустая строка не команда\n");
        } else if (strcmp(buffer, "Exit") == 0) {
            usage = 0;
            printf("Пока-пока\n");
        } else if (strcmp(buffer, "Time") == 0) {
            if (SUCCESS != get_time()) {
                printf("Не удалось получить время!\n");
                if (user->limit != -1) { user->cmd_cnt++; }
            };
        } else if (strcmp(buffer, "Date") == 0) {
            if (SUCCESS != get_date()) {
                printf("Не удалось получить дату!\n");
                if (user->limit != -1) { user->cmd_cnt++; }
            };
        } else if (strcmp(buffer, "Howmuch") == 0) {
            howmuch();
            if (user->limit != -1) { user->cmd_cnt++; }
        } else if (strcmp(buffer, "Logout") == 0) {
            if (user->limit != -1) { user->cmd_cnt++; }
        } else if (strcmp(buffer, "Sanctions") == 0) {
            if (user->limit != -1) { user->cmd_cnt++; }
        } else {
            printf("Нет такой команды\n");
        }

        if (user->limit == user->cmd_cnt) {
            printf("Пора заканчивать! Ты уже сделал свой лимит\n");
            usage = 0;
            printf("Пока-пока\n");
        }
    }

    db_file = fopen("users.db", "wb");
    save_db(&db, db_file);

    free(db.data);
    fclose(db_file);
    return 0;
}
