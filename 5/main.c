#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "base.h"

STATUS_CODE woman_wants_to_enter() {}

STATUS_CODE man_wants_to_enter() {}

STATUS_CODE woman_leaves() {}

STATUS_CODE man_leaves() {}

int main(int argc, char const *argv[]) {
  if (argc != 2) {
    printf("Используй %s <N>\n", argv[0]);
    return INPUT_ERROR;
  }
  long N = atol(argv[0]);
  if (N == LONG_MAX || N == LONG_MIN) {
    return INPUT_ERROR;
  }
  return 0;
}
