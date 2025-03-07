#include "../include/base.h"
#include <stdio.h>
#include <string.h>

#include "msgq.h"

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

    for (;;) {
        if (msgrcv(qid, &msgq, sizeof(msgq.data), 0, 0) == -1) {
            perror("msgsrv qid");
            return INPUT_ERROR;
        }

        printf("%s\n", msgq.data.buf);

        strncpy(msgq.data.buf, "www\0", 4);

        msgq.msg_type = msgq.data.qid;
        msgq.data.qid = qid;

        if (msgsnd (msgq.data.qid, &msgq, sizeof (msgq.data), 0) == -1) {
            perror ("client_qid msgsnd");
            return INPUT_ERROR;
        }
    }
}
