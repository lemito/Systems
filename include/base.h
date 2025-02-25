//
// Created by lemito on 2/23/25.
//

#ifndef BASE_H
#define BASE_H

enum STATUS_CODE {
    SUCCESS = 0,
    INPUT_ERROR,
    NULL_PTR,
    ERROR_OPEN,
    MEMORY_ERROR,
    USER_NOT_CONTAIN,
    USER_IS_CONTAIN,
    SEM_ERR,
    FORK_ERROR,
};

typedef enum STATUS_CODE STATUS_CODE;

#define CLEAR_BUF() while (getchar() != '\n'){}

#endif //BASE_H
