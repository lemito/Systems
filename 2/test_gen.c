#include <stdio.h>

#include "../include/base.h"
//
// Created by lemito on 2/26/25.
//

#define SEED 27022005
#define N 100
#define MASK_NUM 0x16

int main(void) {
  FILE *fp = fopen("meow.txt", "wb");
  if (fp == NULL) {
    return MEMORY_ERROR;
  }
  size_t cnt = 0;
  for (int i = 0; i < N; i++) {
    if ((i & MASK_NUM) == MASK_NUM) {
      fwrite(&i, sizeof(int), 1, fp);
      printf("%d\n", i);
      cnt++;
    }
  }
  fclose(fp);
  printf("Done with %ld\n", cnt);
  return 0;
}
