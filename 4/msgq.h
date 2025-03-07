#ifndef MSGQ_H
#define MSGQ_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MSGQ_KEY "/home/lemito/Desktop/my_sems/msgq"
#define PROJECT_ID 'l'

typedef struct msg_text {
    int qid;
    char buf[35];
  } msg_text;
  
  typedef struct msg {
    long msg_type;
    msg_text data;
  } msg;

#endif