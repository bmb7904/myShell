#include "stdio.h"
#include "string.h"
#include <sys/types.h> // pid_t
#include <sys/wait.h>  // wait
#include <unistd.h>    // fork, execVP
#include <stdlib.h>    // exit
#include <malloc.h>


int main() {

  int fd[2];

  pipe(fd);

  

  pid_t pid1 = fork();

  // child
  if(pid1 == 0) {
    int nums[] = {1, 1, 2, 3, 4};
    // child1
    close(fd[0]);
    
    write(fd[1], nums, sizeof(int) * 5);
    return 0;
    

  }
  // parent 
  else {
    pid_t pid2 = fork();


    if(pid2 == 0) {

      int nums2[5];
      // close write end of pipe
      close(fd[1]);
      read(fd[0], nums2, sizeof(int)*5);
      for(int i = 0; i < 5; i++) {
        printf("%d ", nums2[i]);
      }
      printf("\n");
       return 0;
    }
    else {
      close(fd[1]);
      int num3[5];
      read(fd[0], num3, sizeof(int)*5);
      for(int i = 0; i < 5; i++) {
        printf("%d ", num3[i]);
      }
      printf("\n");

    }

   
    
  }


}