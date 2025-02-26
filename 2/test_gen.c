#include <stdio.h>

#include "../include/base.h"
//
// Created by lemito on 2/26/25.
//

#define SEED 27022005
#define N 10000
#define MASK_NUM 0x27

int main(void) {
    FILE *fp = fopen("test.txt", "wb");
    if (fp == NULL) {
        return MEMORY_ERROR;
    }
    size_t cnt = 0;
    for (unsigned int i = 0; i < N; i++) {
        if (i & MASK_NUM) {
            fwrite(&i, sizeof(unsigned int), 1, fp);
            cnt++;
        }
    }
    fclose(fp);
    printf("Done with %ld\n", cnt);
    return 0;
}
