//
// Created by lemito on 2/23/25.
//

#ifndef BASE_H
#define BASE_H

enum STATUS_CODE {
	SUCCESS = 0,
	INPUT_ERROR = -1,
	NULL_PTR = -2,
	ERROR_OPEN = -3,
	MEMORY_ERROR = -4, USER_NOT_CONTAIN, USER_IS_CONTAIN
};
typedef enum STATUS_CODE STATUS_CODE;

#define CLEAR_BUF() while (getchar() != '\n'){}

#endif //BASE_H
