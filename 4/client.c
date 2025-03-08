#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/base.h"
#include "msgq.h"

// TODO: "идентификация" ???

STATUS_CODE read_cmd(FILE *fin, char **res) {
  if (fin == NULL) {
    return NULL_PTR;
  }
  char c;
  size_t buf_cap = 15;
  size_t buf_size = 0;
  char *buf = (char *)malloc(buf_cap);
  if (buf == NULL) {
    return MEMORY_ERROR;
  }

  if ((c = fgetc(fin)) == EOF) {
    FREE_AND_NULL(buf);
    return SUCCESS;
  }
  ungetc(c, fin);

  while (!isalpha((c = fgetc(fin)))) {
  }
  ungetc(c, fin);

  while ((c = fgetc(fin)) != EOF) {
    if (c == ';') {
      break;
    }
    if (c == '\n' || c == '\t') {
      continue;
    }
    if (buf_size >= buf_cap) {
      buf_cap *= 2;
      char *tmp = (char *)realloc(buf, buf_cap);
      if (tmp == NULL) {
        FREE_AND_NULL(buf);
        return MEMORY_ERROR;
      }
      buf = tmp;
    }
    buf[buf_size++] = (char)tolower(c);
  }
  buf[buf_size] = '\0';

  *res = buf;

  return SUCCESS;
}

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

  if ((strcmp(cmd, "put") == 0 || strcmp(cmd, "move") == 0) && arg != NULL) {
    printf("3");
    return INPUT_ERROR;
  }

  if (strcmp(cmd, "take") == 0 &&
      (arg == NULL || (strcmp(arg, "goat") != 0 && strcmp(arg, "wolf") != 0 &&
                       strcmp(arg, "cabbage") != 0))) {
    printf("4%s\n", cpy);
    return INPUT_ERROR;
  }

  return SUCCESS;
}

int main(const int argc, const char *argv[]) {
  if (argc != 2) {
    printf("Используй %s <файл>\n", argv[0]);
    return INPUT_ERROR;
  }

  if (argv[1] == NULL) {
    return NULL_PTR;
  }

  FILE *file = NULL;
  char *line;
  key_t msgq_key;
  int server_qid, client_qid;
  msg client_msg, server_msg;
  int st = 0;
  char auth = 1;

  for (size_t i = 0; i < 35; i++) {
    client_msg.data.buf[i] = 0;
    server_msg.data.buf[i] = 0;
  }

  if ((client_qid = msgget(IPC_PRIVATE, 0660)) == -1) {
    perror("msgget client_qid\n");
    return INPUT_ERROR;
  }

  if ((msgq_key = ftok(MSGQ_KEY, PROJECT_ID)) == -1) {
    perror("ftok msgq_key\n");
    return INPUT_ERROR;
  }

  if ((server_qid = msgget(msgq_key, 0660)) == -1) {
    perror("msgget server_qid\n");
    return INPUT_ERROR;
  }

  file = fopen(argv[1], "r");
  if (file == NULL) {
    perror("Файл");
    return ERROR_OPEN;
  }

  client_msg.msg_type = 1;
  client_msg.data.qid = client_qid;

  // char line[27];

  while (!feof(file)) {
    st = read_cmd(file, &line);
    const size_t slen = strlen(line);
    if (st != SUCCESS || slen == 0) {
      FCLOSE(file);
      FREE_AND_NULL(line);
      return st;
    }
    if (slen > 15 || slen < 3) {
      printf("Введена неизвестная команда!\n");
      return INPUT_ERROR;
    }
    if (line[slen - 1] == '\n') line[slen - 1] = '\0';
    // while (line = myfgets(file)){

    // puts(line);

    /// client_msg.data.buf - команда

    char *cpy = strdup(line);
    if (cpy == NULL) {
      FREE_AND_NULL(line);
      FCLOSE(file);
      return MEMORY_ERROR;
    }

    st = cmd_check(cpy);
    if (st == INPUT_ERROR) {
      printf("Неправильный ввод!\n");
      FREE_AND_NULL(line);
      FCLOSE(file);
      FREE_AND_NULL(cpy);
      return st;
    }

    if (auth) {
      snprintf(client_msg.data.buf, 35, "%d", getpid());
    }

    // // подготовк к выводу
    strcpy(client_msg.data.buf, line);

    // вывод
    if (msgsnd(server_qid, &client_msg, sizeof(client_msg.data), 0) == -1) {
      perror("client_qid msgsnd\n");
      FREE_AND_NULL(line);
      FCLOSE(file);
      FREE_AND_NULL(cpy);
      return INPUT_ERROR;
    }
    FREE_AND_NULL(cpy);
  }
  // чтение с сервера
  // if (msgrcv(server_qid, &server_msg, sizeof(server_msg.data), 0, 0) == -1) {
  //   perror("server_qid msgrcv");
  //   FREE_AND_NULL(line);
  //   FCLOSE(file);
  //   // FREE_AND_NULL(cpy);
  //   return INPUT_ERROR;
  // }

  FREE_AND_NULL(line);
  // FREE_AND_NULL(cpy);

  FCLOSE(file);

  if (msgctl(client_qid, IPC_RMID, NULL) == -1) {
    perror("msgctl client_qid");
    FCLOSE(file);
    return INPUT_ERROR;
  }

  return 0;
}
