#include "../include/base.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "msgq.h"

STATUS_CODE cmd_check(char *cpy) {
    if (cpy == NULL) { return NULL_PTR; }

    const char *cmd = strtok(cpy, " ");
    const char *arg = strtok(NULL, " ");
    const char *garbage = strtok(NULL, " ");

    if (garbage != NULL || cmd == NULL) {
        printf("1");
        return INPUT_ERROR;
    }

    if (strcmp(cmd, "take") != 0 &&
        strcmp(cmd, "put") != 0 &&
        strcmp(cmd, "move") != 0) {
        printf("%s\n", cpy);
        return INPUT_ERROR;
    }

    if ((strcmp(cmd, "put") == 0 || strcmp(cmd, "move") == 0) && arg != 0) {
        printf("3");
        return INPUT_ERROR;
    }

    if (strcmp(cmd, "take") == 0 && (arg == NULL || (
                                         strcmp(arg, "goat") != 0 && strcmp(arg, "wolf") != 0 && strcmp(arg, "cabbage")
                                         !=
                                         0))) {
        printf("%s\n", cpy);
        return INPUT_ERROR;
    }

    return SUCCESS;
}

int main() {
    key_t msgq_key;
    int qid;
    msg msgq;

    if ((msgq_key = ftok(MSGQ_KEY, PROJECT_ID)) == -1) {
        perror("ftok msgq_key\n");
        return INPUT_ERROR;
    }

    if ((qid = msgget(msgq_key, IPC_CREAT | 0660)) == -1) {
        perror("msgget server_qid\n");
        return INPUT_ERROR;
    }

    // первый берег -- начальный. вначале там все
    // 0 - никто; 1 - волк, 2 - коза, 3 - капуста
    char first_bereg[3] = {1, 2, 3};
    // второй берег -- изначально там никого нет
    char second_bereg[3] = {0, 0, 0};
    // берег на котором сейчас лодка
    char cur_bereg = 0; // 0 == 1; 1 == 2
    char boat = 0;

    for (;;) {
        /**
        =====================
        Решение:
        Перевезти козу
        Вернуться
        Перевезти волка (или капусту)
        Вернуться с козой
        Перевезти капусту (или волка)
        Вернуться
        Перевезти козу


        take goat
        move
        put
        move
        take wolf
        move
        put
        take goat
        move
        put
        take cabagge
        move
        put
        move
        take goat
        move
        put
        =====================
        GC/CG == BAN
        GW/WG == BAN
         */


        if (msgrcv(qid, &msgq, sizeof(msgq.data), 0, 0) == -1) {
            perror("msgsrv qid");
            return INPUT_ERROR;
        }

        printf("%s\n", msgq.data.buf);

        char *cpy = strdup(msgq.data.buf);
        if (cpy == NULL) {
            return MEMORY_ERROR;
        }
        msgq.msg_type = msgq.data.qid;
        msgq.data.qid = qid;
        const STATUS_CODE st = cmd_check(cpy);
        if (st == INPUT_ERROR) {
            const char msg[] = "Команда неверна\0";
            strncpy(msgq.data.buf, msg, strlen(msg));

            if (msgsnd(msgq.data.qid, &msgq, sizeof (msgq.data), 0) == -1) {
                perror("client_qid msgsnd");
                FREE_AND_NULL(cpy);
                return INPUT_ERROR;
            }
        }
        FREE_AND_NULL(cpy);

        const char *cmd = strtok(cpy, " ");
        const char *arg = strtok(NULL, " ");

        /// обработка команд (валидация на клиенте, поэтому считаем команды хорошими)
        if (strcmp(cmd, "move") == 0) {
            // меняем берег
            cur_bereg = cur_bereg == 0 ? 1 : 0;
        } else if (strcmp(cmd, "put") == 0) {
            // опусташаем содержимое лодки на берег
            switch (cur_bereg) {
                case 0: {
                    first_bereg[boat - 1] = boat;
                }
                break;
                case 1: {
                    second_bereg[boat - 1] = boat;
                }
                break;
                default: {
                }
                break;
            }
        } else if (strcmp(cmd, "take") == 0) {
            // берем с берега кого-либо и кладем в лодку
            if (strcmp(arg, "wolf") == 0) {
                // волк
                switch (cur_bereg) {
                    case 0: {
                        boat = first_bereg[0];
                    }
                    break;
                    case 1: {
                        boat = second_bereg[0];
                    }
                    break;
                    default: {
                    }
                    break;
                }
            } else if (strcmp(arg, "goat") == 0) {
                // коза
                switch (cur_bereg) {
                    case 0: {
                        boat = first_bereg[1];
                    }
                    break;
                    case 1: {
                        boat = second_bereg[1];
                    }
                    break;
                    default: {
                    }
                    break;
                }
            } else if (strcmp(arg, "cabbage") == 0) {
                // капуста
                switch (cur_bereg) {
                    case 0: {
                        boat = first_bereg[2];
                    }
                    break;
                    case 1: {
                        boat = second_bereg[2];
                    }
                    break;
                    default: {
                    }
                    break;
                }
            }
        }

        // список плохих ситуаций - коза-капуста x2, волк-коза x2
        const char bans[][] = {{2, 3}, {3, 2}, {1, 2}, {2, 1}};

        if (strstr(first_bereg, bans[0]) != NULL || strstr(first_bereg, bans[1]) != NULL) {
            printf("Коза съела капусту :(\n");
        } else if (strstr(first_bereg, bans[2]) != NULL || strstr(first_bereg, bans[3]) != NULL) {
            printf("Волк съел козу :(\n");
        }

        // const char msg[] = "Мяу!\0";
        // strncpy(msgq.data.buf, msg, strlen(msg));
        //
        // if (msgsnd(msgq.data.qid, &msgq, sizeof (msgq.data), 0) == -1) {
        //     perror("client_qid msgsnd");
        //     FREE_AND_NULL(cpy);
        //     return INPUT_ERROR;
        // }

        FREE_AND_NULL(cpy);
    }
}
