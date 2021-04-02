#include "stdio.h"
#include "string.h"
#include <sys/types.h> // pid_t
#include <sys/wait.h>  // wait
#include <unistd.h>    // fork, execVP
#include <stdlib.h>    // exit
#include <malloc.h>
#include <fcntl.h>
#include <errno.h> 

int main(int argc, char** argv) {
  FILE* fp = fopen("input.txt", "r");
  write(1, ">>", 2);

  while(1) {
    char a = getc(fp);
    if(a == EOF) {
      printf("EOF FOUND\n");
      int x;
      if( (x = feof(fp)) != 0) {
        printf("%d %c\n", x, x);
        break;
      }
    }
    else {
      printf("%c", a);
    }
  }
  
}