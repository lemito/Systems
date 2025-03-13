#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/base.h"
#include "msgq.h"

enum POSITIONS { WOLF = 0, GOAT = 1, CABBAGE = 2 };
enum ANIMALS { NOTHING = 0, WOLF_A = 1, GOAT_A = 2, CABBAGE_A = 3 };

STATUS_CODE cmd_check(char *cpy) {
  if (cpy == NULL) {
    return NULL_PTR;
  }

  const char *cmd = strtok(cpy, " ");
  const char *arg = strtok(NULL, " ");
  const char *garbage = strtok(NULL, " ");

  if (garbage != NULL || cmd == NULL) {
    printf("1");
    return INPUT_ERROR;
  }

  if (strcmp(cmd, "take") != 0 && strcmp(cmd, "put") != 0 &&
      strcmp(cmd, "move") != 0) {
    printf("%s\n", cpy);
    return INPUT_ERROR;
  }

  if ((strcmp(cmd, "put") == 0 || strcmp(cmd, "move") == 0) && arg != 0) {
    printf("3");
    return INPUT_ERROR;
  }

  if (strcmp(cmd, "take") == 0 &&
      (arg == NULL || (strcmp(arg, "goat") != 0 && strcmp(arg, "wolf") != 0 &&
                       strcmp(arg, "cabbage") != 0))) {
    printf("%s\n", cpy);
    return INPUT_ERROR;
  }

  return SUCCESS;
}

// здесь могла быть хэш0табличка или список, но будет дин. массив
typedef struct db {
  pid_t *data;
  size_t siz;
  size_t cap;
} db_t;

/// АЛИАСЫ ДЛЯ ЧЕКЕРА
#define INSERTED (~6)
#define FINDED (~5)

STATUS_CODE check_or_insert(db_t *db, pid_t pid) {
  if (NULL == db) {
    return NULL_PTR;
  }
  if (db->siz == 0 || db->cap == 0) {
    return INPUT_ERROR;
  }

  for (size_t i = 0; i < db->siz; i++) {
    if (db->data[i] == pid) {
      return FINDED;
    }
  }

  if (db->cap >= db->siz) {
    pid_t *tmp = realloc(db->data, db->cap * 2);
    if (NULL == tmp) {
      FREE_AND_NULL(db->data);
      return MEMORY_ERROR;
    }
    db->data = tmp;
    db->cap *= 2;
  }
  db->data[db->siz++] = pid;

  return INSERTED;
}

STATUS_CODE db_remove(db_t *db) {
  if (db == NULL) {
    return NULL_PTR;
  }
  db->cap = db->siz = 0;
  FREE_AND_NULL(db->data);
  return SUCCESS;
}

int main() {
  key_t msgq_key;
  int qid;
  msg msgq;
  db_t db;
  db.cap = 5;
  db.siz = 0;
  db.data = malloc(db.cap * sizeof(pid_t));
  if (db.data == NULL) {
    return MEMORY_ERROR;
  }

  if ((msgq_key = ftok(MSGQ_KEY, PROJECT_ID)) == -1) {
    perror("ftok msgq_key\n");
    db_remove(&db);
    return INPUT_ERROR;
  }

  if ((qid = msgget(msgq_key, IPC_CREAT | IPC_EXCL | 0660)) == -1) {
    // perror("msgget server_qid. Try///");
    if (qid == -1) {
      qid = msgget(msgq_key, 0666);
      msgctl(qid, IPC_RMID, NULL);
      qid = msgget(msgq_key, 0666 | IPC_CREAT);
      if (qid == -1) {
        perror("msgget server_qid\n");
        db_remove(&db);
        return INPUT_ERROR;
      }
    }
    // return INPUT_ERROR;
  }

  // первый берег -- начальный. вначале там все
  // 0 - никто; 1 - волк, 2 - коза, 3 - капуста
  char first_bereg[3] = {WOLF_A, GOAT_A, CABBAGE_A};
  // второй берег -- изначально там никого нет
  char second_bereg[3] = {NOTHING, NOTHING, NOTHING};
  // берег на котором сейчас лодка
  char cur_bereg = 0;  // 0 == 1; 1 == 2
  char boat = 0;
  char buf[512] = {0};

  // список плохих ситуаций - коза-капуста x2, волк-коза x2
  const char bans[4][2] = {{GOAT_A, CABBAGE_A},
                           {CABBAGE_A, GOAT_A},
                           {WOLF_A, GOAT_A},
                           {GOAT_A, WOLF_A}};

  // условие "выигрыша" - первый берег пуст, а второй - с вещами
  const char NULL_F[3] = {NOTHING, NOTHING, NOTHING};
  const char RES[4] = {WOLF_A, GOAT_A, CABBAGE_A, 0};

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
    take cabbage
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
      db_remove(&db);
      return INPUT_ERROR;
    }

    printf("Сейчас мы работаем с клиентом №%ld и обрабатываем %s\n",
           msgq.msg_type, msgq.data.buf);

    int check_st = check_or_insert(&db, msgq.msg_type);
    if (check_st == MEMORY_ERROR) {
      db_remove(&db);
      return MEMORY_ERROR;
    } else if (check_st == NULL_PTR) {
      db_remove(&db);
      return NULL_PTR;
    }

    char *cpy = strdup(msgq.data.buf);
    if (cpy == NULL) {
      db_remove(&db);
      return MEMORY_ERROR;
    }

    const STATUS_CODE st = cmd_check(cpy);
    msgq.msg_type = 1;
    msgq.data.qid = qid;
    if (st == INPUT_ERROR) {
      const char msg[] = "Команда неверна\0";
      strncpy(msgq.data.buf, msg, strlen(msg));
      printf("%s\n", msg);
      if (msgsnd(msgq.data.qid, &msgq, sizeof(msgq.data), 0) == -1) {
        perror("client_qid msgsnd");
        FREE_AND_NULL(cpy);
        db_remove(&db);

        return INPUT_ERROR;
      }
    }
    FREE_AND_NULL(cpy);

    cpy = strdup(msgq.data.buf);
    if (cpy == NULL) {
      db_remove(&db);
      return MEMORY_ERROR;
    }

    const char *cmd = strtok(cpy, " ");
    const char *arg = strtok(NULL, " ");

    /// обработка команд (валидация на клиенте, поэтому считаем команды
    /// хорошими)
    if (strcmp(cmd, "move") == 0) {
      // меняем берег
      cur_bereg = cur_bereg == 0 ? 1 : 0;
    } else if (strcmp(cmd, "put") == 0) {
      if (boat == 0) {
        strcpy(buf, "Лодка пуста");
        break;
      }
      // опусташаем содержимое лодки на берег
      switch (cur_bereg) {
        case 0: {
          first_bereg[boat - 1] = boat;
          boat = 0;
        } break;
        case 1: {
          second_bereg[boat - 1] = boat;
          boat = 0;
        } break;
        default: {
        } break;
      }
    } else if (strcmp(cmd, "take") == 0) {
      if (boat != 0) {
        strcpy(buf, "Лодка не пуста");
        break;
      }
      // берем с берега кого-либо и кладем в лодку
      if (strcmp(arg, "wolf") == 0) {
        // волк
        switch (cur_bereg) {
          case 0: {
            boat = first_bereg[0];
            first_bereg[0] = 0;
          } break;
          case 1: {
            boat = second_bereg[0];
            second_bereg[0] = 0;
          } break;
          default: {
          } break;
        }
      } else if (strcmp(arg, "goat") == 0) {
        // коза
        switch (cur_bereg) {
          case 0: {
            boat = first_bereg[1];
            first_bereg[1] = 0;
          } break;
          case 1: {
            boat = second_bereg[1];
            second_bereg[1] = 0;
          } break;
          default: {
          } break;
        }
      } else if (strcmp(arg, "cabbage") == 0) {
        // капуста
        switch (cur_bereg) {
          case 0: {
            boat = first_bereg[2];
            first_bereg[2] = 0;
          } break;
          case 1: {
            boat = second_bereg[2];
            second_bereg[2] = 0;
          } break;
          default: {
          } break;
        }
      }
    }

    if (strstr(first_bereg, bans[0]) != NULL ||
        strstr(first_bereg, bans[1]) != NULL ||
        strstr(second_bereg, bans[0]) != NULL ||
        strstr(second_bereg, bans[1]) != NULL) {
      // printf("Коза съела капусту :(\n");
      strcat(buf, "Коза съела капусту :(\n");
    } else if (strstr(first_bereg, bans[2]) != NULL ||
               strstr(first_bereg, bans[3]) != NULL ||
               strstr(second_bereg, bans[2]) != NULL ||
               strstr(second_bereg, bans[3]) != NULL) {
      // printf("Волк съел козу :(\n");
      strcat(buf, "Волк съел козу :(\n");
    } else if (strcmp(first_bereg, NULL_F) == 0 &&
               strcmp(second_bereg, RES) == 0) {
      // printf("Все счастливо переплыли на другой берег\n");
      strcat(buf, "Успех!\n");
      FREE_AND_NULL(cpy);
      break;
    }

    // const char msg[] = "Мяу!\0";

    FREE_AND_NULL(cpy);
  }

  strncpy(msgq.data.buf, buf, strlen(buf));

  for (size_t i = 0; i < db.siz; i++) {
    if (msgsnd(db.data[i], &msgq, sizeof(msgq.data), 0) == -1) {
      perror("client_qid db.data[i] msgsnd after work");
      db_remove(&db);
      return INPUT_ERROR;
    }
  }

  printf("%s\n", buf);
  db_remove(&db);
  return 0;
}
