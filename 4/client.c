#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/base.h"
#include "msgq.h"

char *myfgets(FILE *file) {
  size_t size = 0;
  size_t cap = 15;
  char *res = (char *)malloc(cap * sizeof(char));
  char c;
  if ((c = fgetc(file)) == '\n') {
    strcpy(res, "\n");
    return res;
  }
  ungetc(c, file);
  while ((c = fgetc(file)) != EOF && c != '\n') {
    res[size] = c;
    if (size >= cap) {
      cap *= 2;
      char *new = (char *)realloc(res, sizeof(char) * cap);
      if (new == NULL) {
        FREE_AND_NULL(new);
        return NULL;
      }
      res = new;
    }
    size++;
  }
  res[size] = '\0';
  if (strcmp(res, "\0") == 0) {  //  || strcmp(res, "\n") == 0
    FREE_AND_NULL(res);
    return NULL;
  }
  return res;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Используй %s <файл>\n", argv[0]);
    return INPUT_ERROR;
  }

  if (argv[1] == NULL) {
    return NULL_PTR;
  }

  FILE *file = NULL;
  char *line = NULL;
  key_t msgq_key;
  int server_qid, client_qid;
  msg client_msg, server_msg;

  if ((client_qid = msgget(IPC_PRIVATE, 0660)) == -1) {
    perror("msgget client_qid\n");
    return INPUT_ERROR;
  }

  if ((server_qid = ftok(MSGQ_KEY, PROJECT_ID)) == -1) {
    perror("ftok msgq_key\n");
    return INPUT_ERROR;
  }

  if ((server_qid = msgget(IPC_PRIVATE, 0660)) == -1) {
    perror("msgget server_qid\n");
    return INPUT_ERROR;
  }

  file = fopen(argv[1], "r");
  if (file == NULL) {
    return ERROR_OPEN;
  }

  client_msg.msg_type = 1;
  client_msg.data.qid = client_qid;

  while (!feof(file) && (line = myfgets(file))) {
    puts(line);
    // подготовк к выводу
    strncpy(client_msg.data.buf, line, strlen(line));

    // вывод
    if (msgsnd(client_qid, &client_msg, sizeof(client_msg), 0) == -1) {
      perror("client_qid msgsnd\n");
      return INPUT_ERROR;
    }

    // чтение с сервера
    if (msgrcv(server_qid, &server_msg, sizeof(server_msg), 0, 0) == -1) {
      perror("server_qid msgrcv");
      return INPUT_ERROR;
    }



    FREE_AND_NULL(line);
  }

  FCLOSE(file);

  if (msgctl(client_qid, IPC_RMID, NULL) == -1) {
    perror("msgctl client_qid");
    return INPUT_ERROR;
  }

  return 0;
}