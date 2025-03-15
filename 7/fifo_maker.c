#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>

#define NAME1 "meow"
#define NAME2 "meow2"

int main(int argc, char const *argv[]) {
  if (mkfifo(NAME1, 0666) == -1) {
    perror("fifo open err");
    return 1;
  }
  if (mkfifo(NAME2, 0666) == -1) {
    perror("fifo open err");
    return 1;
  }
  return 0;
}
