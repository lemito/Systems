//
// Created by lemito on 2/23/25.
// https://pubs.opengroup.org/onlinepubs/009604599/basedefs/dirent.h.html
//
/*
 * /.../cmake-debug/meow1/meow.txt
 * meow - имя каталога; не путь (относительный или абсолютный)!
 */
#include <stdio.h>
#include <dirent.h>
#include "../include//base.h"

STATUS_CODE dir_browse(const char *path) {
	if (path == NULL) {
		return NULL_PTR;
	}

	DIR *dirp = NULL; // тек. директория
	struct dirent *dentry = NULL; // инфа о ней

	if ((dirp = opendir(path)) == NULL) {
		fprintf(stderr, "opendir ошибка\n");
		closedir(dirp);
		return ERROR_OPEN;
	}

	while ((dentry = readdir(dirp)) != NULL) {
		char name[PATH_MAX];
		snprintf(name, sizeof(name) / sizeof(char), "%s/%s", path, dentry->d_name);

		switch (dentry->d_type) {
			case DT_DIR: {
				printf("директория: %s\n", name);
			}
			break;
			case DT_REG: {
				printf("файл: %s\n", name);
			}
			break;
			case DT_LNK: {
				printf("симв. ссылка: %s\n", name);
			}
			break;
			case DT_UNKNOWN: {
				printf("?: %s\n", name);
			}
			break;
			case DT_BLK: {
				printf("диск (блочное устройство): %s\n", name);
			}
			break;
			case DT_CHR: {
				printf("диск (символьное устройство): %s\n", name);
			}
			break;
			case DT_FIFO: {
				printf("именнованный канал: %s\n", name);
			}
			break;
			case DT_SOCK: {
				printf("сокет: %s\n", name);
			}
			break;
			default: {
				fprintf(stderr, "err");
			}
			break;
		}
	}

	closedir(dirp);
	return SUCCESS;
};

int main(const int argc, char *argv[]) {
	if (argc < 2) {
		printf("Используй: %s <path1> <path2> ... <pathn>\n", argv[0]);
		return INPUT_ERROR;
	}

	for (int i = 1; i < argc; i++) {
		if (argv[i] == NULL) {
			return INPUT_ERROR;
		}

		printf("%s\n", argv[i]);
		const STATUS_CODE status = dir_browse(argv[i]);
		if (status != SUCCESS) { return status; }
		printf("===\n");
	}

	return 0;
}
