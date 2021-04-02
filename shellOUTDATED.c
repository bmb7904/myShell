/*
 * There are two ways to create a process in Linux. One is to call init(),
 * which is called at startup and it begins the kernel. The other is to fork
 * a currently existing process. The fork() function will fork the caller and
 * create a new child process. This child process is identical to the parent
 * process. The parent's entire context is cloned, including all
 * registers including its program counter, which means that the new child
 * process begins being executed at the exact same location in the machine
 * instructions as the parent was when fork was called. We can use fork() to
 * create child processes, and then use the exec family of functions to
 * overwrite the child's memory address with a new process.

 * Rules for my shell:
 1.) Proper spacing between commands and arguments is absolutely crucial. I am not smart enough to write a shell that can interpret what you are were "trying" to say.

 */

#include "stdio.h"
#include "string.h"
#include <sys/types.h> // pid_t
#include <sys/wait.h>  // wait
#include <unistd.h>    // fork, execVP
#include <stdlib.h>    // exit
#include <malloc.h>
#include <fcntl.h>

char* extractLine();
int argCounter(char* buffer);
char** tokenize(char* buffer, int numArgs);
int contains_ampersand(char** argArray);
int contains_exit(char** argArray);
void exit_program();
int getPipe(char** argArray);
void external_process(char** argArray, int numArgs, int bg_flag);
void argError();
void pipeProcesses(char** argArray, int pipe_index);
int contains_cd(char** argArray);
void cdCommand(char** argArray);
int contains_redirection_sym(char** argArray);
int redirect_output(char** argArray, int redir_index);
void closeRedirection(int o);

int numArgs;
int counter;


int main(int argc, char** argv) {

// Must use "\\"" to print out a single "\" in printf()

printf("--------------------------------------------------------------\n");

printf("                         /)                      \n");
printf("                /\\___/\\ ((                       \n");
printf("                \\`@_@'/  ))                      \n");
printf("                {_:Y:.}_//                       \n");
printf("    --Brett's--{_}^-'{_}----Shell---             \n");
printf("--------------------------------------------------------------\n\n");

    // the line of user input to be parsed and tokenized
    char *buffer;
    // an array of strings. holds each individual argument input by user
    char **argArray;
    // index of the pipe in the argArray (if a pipe exits). If no pipe, set to -1.
    int pipe_index = -1;

    int contains_pipe;
    int redir_index = -1; // index is -1 if no "<" or "<<"
    int bg_flag = 0;

    int x = 0;

    struct fileRedir { 
      int numOfSymbols; 
      int index;
    };

    // The main loop for the shell. Only breaks out if the exit command is
    // called by the user. There are ? general cases, dependent on user
    // input. Case 1: The user specifies an external process, executed
    // outside the shell. 2.) the user inputs "cd" command, which will
    // utilize the built-in cd command. 3.) The user wants to use a pipe, to
    // pipe output of one command into another. 4.) User enters the "exit"
    // command"
    // A subset of case 1,
    // utilizaing an external process, has 2 sub-categories: running the
    // process in the foreground or the background. This is simply a
    // difference of having the parent process wait for the child to
    // terminate or not.
    while(1) {

        write(1, "\n>> ", 4);
        buffer = extractLine();
        numArgs = argCounter(buffer);
        argArray = tokenize(buffer, numArgs);


        if(contains_exit(argArray) == 1) {
            write(1, "Good-bye!\n", 10);
            exit(0);
        }

        if( contains_cd(argArray) == 1) {
            cdCommand(argArray);
        } 
        else {
            pipe_index = getPipe(argArray);

            if(contains_ampersand(argArray)) {
                bg_flag = 1;
            }
            else {
                bg_flag = 0;
            }

            redir_index = contains_redirection_sym(argArray);
            
            // if file is to have redirected output, set it up before
            // executing processes
            if(redir_index != -1) {
              printf("YES\n");
               x = redirect_output(argArray, redir_index);
            }

            if(pipe_index != -1) {
                // do someting here
                // for now,
                //write(1, "\nContains pipe!\n", 16);
                pipeProcesses(argArray, pipe_index);
            }
            else {
                external_process(argArray, numArgs, bg_flag);
            }
      }



        if(x != 0) {
          closeRedirection(x);
        }
        printf("HERE333");

        free(buffer);
        free(argArray);

    }

}



// if &, the parent doesn't use wait(). It means run in background.
// use for cd.
//    chdir()

/**
 * Takes the entire line of user input, stores it into a string (char array)
 * and returns the array. Doing it this way allows for dynamic reallocation
 * of memory, and thus, no guessing on how large user input can or will be.
 * @return
 */
char* extractLine() {
    int size_buffer = 8;
    counter = 0;
    char *buffer;
    buffer = malloc(sizeof(char) * size_buffer);

    int index_into_buffer = 0;


    while(1) {
        char c = getchar();
        if (c == '\n') {
            // Add terminating char at end of buffer.
            buffer[index_into_buffer] = '\0';
            return buffer;
        }

        buffer[index_into_buffer] = c;
        index_into_buffer++;
        counter++;

        if (index_into_buffer >= size_buffer) {
            size_buffer *= 2;
            // increase size of buffer. 
            // returns a pointer to newly allocated memory
            buffer = realloc(buffer, (sizeof(char) * size_buffer));
           
        }
    }
}

/*
 * Counts number of args by counting spaces in the input string buffer
 */
int argCounter(char* buffer) {

    int counter = 0;
    for(char* p = buffer; *(p) != EOF && *(p) != '\0'; p++) {
        if( *(p) == ' ') {
            counter++;
        }
    }


    return counter + 1;


}

/*
 * Takes the entire line of user input as a string, and tokenizes it. 
 * Also takes the number of arguments contained in the string. Stores each token (separated by spaces) into
 * an array of strings. Returns the array of strings (Actually an array of char* pointers).
 */
char** tokenize(char* buffer, int numArgs) {
    int token_index = 0;

    char** argArray;

    // Now that we know how many arguments, allocate memory for each pointer
    // in the array of char* pointers. We don't need to allocate memeory for each
    // string(inner char array) in the (outer) array, as they will remain stored in the memory location
    // they are currently in (currently pointed at by char* buffer).
    argArray = malloc((sizeof(char*)) * numArgs + 1);

    /*
     * The C library function char *strtok(char *str, const char *delim)
     * breaks the string into a series of tokens using the specified delimiter .
     * This function returns a pointer to the first token found in the
     * string. A null pointer is returned if there are no tokens left to retrieve.
     * This does not create new strings in memory, it simply tokenizes the each string
     * by adding a '\0' character at the end of each token and returns a pointer to each token.     
     */
    char *token = strtok(buffer, " ");
    // loop through the string to extract all other tokens
    while( token != NULL ) {
        *(argArray + token_index) = token;
        token = strtok(NULL, " ");

        token_index++;
    }

    // terminate the char pointer array(char ** argArray) with a NULL pointer.
    // This is absolutely crucial. Otherwise, after multiple commands entered by the user in
    // the while loop, there will be garbage in memory that will cause execvp() to not function properly.
    // Since execvp() is expected a null pointer at the end of the array, we can simply
    // ignore the garbage. 
    argArray[token_index] = NULL;


    return argArray;
}

/**
 * Function that takes in an array of strings, searches each token for any
 * instance of the '&' character, and returns 1 if the character is found, 0
 * otherwise. If the '&' char is found, it is removed from the char** array. 
 * strcmp() is used: this works by taking two pointers to char arrays as
 * arguments, and returns 0 if the two strings are equal
 */
int contains_ampersand(char** argArray) {
    for(char** p = argArray ; *(p) != NULL; p++){
        if(strcmp(*(p), "&") == 0) {
          *(p) = NULL; // take off '&' in argument array
          numArgs--; // subtract one from number of arguments. 
          return 1;
        }
    }
    return 0;
}

/**
 * Function that takes in an array of strings, searches each string for any 
 * redirected output symbol ">>" or ">". If none are found, function returns 0. 
 * If a single ">" argument is found, return 1. If two ">>" are found, return 2
 * These two commands are related by have slightly different properties. 
 */
int contains_redirection_sym(char** argArray) {
    int i = 0;
    for(char** p = argArray ; *(p) != NULL; p++){
        if(strcmp(*(p), ">") == 0) {
          return i;
        }
        if(strcmp(*(p), ">>") == 0) {
          return i;
        }
        i++;
    }
  return -1;
    
}

int  redirect_output(char ** argArray, int redir_index) {
    char* fileName = argArray[redir_index + 1]; // get file name
    if(strcmp(argArray[redir_index], ">") == 0) {
      argArray[redir_index] = NULL;
      close(1);
      int output = open(fileName, O_CREAT | O_WRONLY | O_TRUNC, 0666);
      
      dup2(output, 1);
      return output;
    }
    else if(strcmp(argArray[redir_index], ">>") == 0) {
      argArray[redir_index] = NULL;
      argArray[redir_index] = NULL;
      close(1);
      int output = open(fileName, O_APPEND | O_WRONLY | O_TRUNC, 0666);
      dup2(output, 1);
      return output;
    }
    else {
      // shouldn't ever get here, but just in case
      argError();
    }
}

void closeRedirection(int o) {
  printf("GAY\n");
  close(o);
  dup2(stdout, o);
  
}

/**
 * Function that takes in an array of strings, searches each token for any
 * instance of the "cd" command, and returns 1 if the command is found, 0
 * otherwise. strcmp() is used. Takes two pointers to char arrays(strings) as
 * arguments, and returns 0 if the two strings are equal
 */
int contains_cd(char** argArray) {
    for(char** p = argArray ; *(p) != NULL; p++){
        if(strcmp(*(p), "cd") == 0) {
          *(p) = NULL; 
          return 1;
        }
    }
    return 0;
}

/*
 * Built-in "cd" command that will change the working directory of the parent process. No forking needed. 
 */
void cdCommand(char** argArray) {
    if(chdir(argArray[1]) != 0) {
      argError();
    }
}

/*
 * Will exit out of the program.
 */
void exit_program(){
    exit(0);
}

/*
 * Checks if the first command was "exit".  strcmp() is used. Take two points
 * to char arrays as arguments, and returns 0 if the two
 */
int contains_exit(char** argArray) {
    for(char** p = argArray ; *(p) != NULL; p++){
        if(strcmp(*(p), "exit") == 0) {
            return 1;
        }
    }
    return 0;
    return 0;
}
/*
 * Checks array of arguments for a pipe symbol '|'. If it exits, return the
 * index of the pipe in the argArray supplied by the parameter. If no pipe present,
 * return -1;
 */
int getPipe(char** argArray) {
    int i = 0;
    for(char** p = argArray ; *(p) != NULL; p++){
        if(strcmp(*(p), "|") == 0) {
            return i;
        }
        i++;
    }

    return -1;
}

void external_process(char** argArray, int numArgs, int bg_flag) {
    /*
 * The execvp() takes the file name of the program you wish to use to overwrite
 * the caller process as the first argument, followed by an
 * array of the arguments supplied by the user to that program. This is opposed to
 * execlp(), which takes each user argument as separate arguments. Char** array(array of strings)
 * must be terminated with a NULL pointer. Instead of using the
 * relative or absolute path of the desired program, members of the EXEC family of functions that
 * contain a "p" in the name (like execvp) will search for the correct path given the correct
 * program name. For example, simply using "ls" as the first argument,
 * the function searches and finds /bin/ls.
 */

    pid_t pid = fork();

    // if user specified to run process in background
    if(bg_flag == 1) {
        if(pid == 0) {
            execvp(argArray[0], argArray);
            argError();
        }
        else {
            // this is needed to prevent staggered output from the background child process 
            // interwoven with the parent. 
            sleep(1);
        }

    }
    // process run in foreground
    else {
        // child process
        if(pid == 0) {

            execvp(argArray[0], argArray);
            argError();
        }
        // parent
        else {
            wait(NULL); // wait for child to terminate 
            //printf("Child done\n");
        }
    }

}
/*
 * Writes Error message to stdout
 */
void argError(){
    write(1, "Invalid Arguments! Try again.\n", 31);
    exit(1);
}

/*
 * Preconditions: User commands as an array of strings (char**) containing two external commands separated
 * by the '|' pipe symbol, at the specified pipe_index. The output of the command on the left of the '|' will be 
 * piped into the input of the second command. This function will separate the commands on the 
 * left and right side of the pipe. The pipe will be set up before any forking, and there will be two
 * child processes needed, as simply using one child and the parent will result in the shell(the parent)
 * terminating after execution. In other words, we don't want to use the parent to execute any external 
 * commands(with execvp), as we can never gain control back and continue with the shell. 
 *
 *
 */
void pipeProcesses(char** argArray, int pipe_index) {
  char** first_command =  malloc(sizeof(char*) * (pipe_index + 1)); //  (output goes to pipe);
  char** second_command = malloc(sizeof(char*) * (numArgs - pipe_index)); //  (input comes from pipe);
  // array of file descriptors
  int fd[2];
  pid_t pid;

  for(int i = 0; i < pipe_index; i++) {
    first_command[i] = argArray[i];
  }

  *(first_command + pipe_index) = NULL;

  int k = pipe_index + 1;

  for(int j = 0; j <= (numArgs - pipe_index - 1); j++) {
    if(j == (numArgs - pipe_index - 1)) {
      second_command[j] = NULL; // add null pointer at end of array
    }
    else {
      second_command[j] = argArray[j + pipe_index + 1];
    }
    
  }


  pipe(fd); // sets up pipe in kernel space and adds file descriptors to the fd[] array argument.

  pid = fork(); // fork current process

  // if true, this process is the first child. 
  if(pid == 0) {
        // overwrite slot 1 (stdout) of FD table of child process with the write end of the pipe. 
        // Child's process will write to pipe instead of stdout
        dup2(fd[1], 1);

        // close read end of pipe
        close(fd[0]);

        execvp(first_command[0], first_command);
        argError();
        exit(0); // shouldn't get here

  }
  // The parent process. 
  else {

    // fork again
    pid_t pid2 = fork();

    // second child 
    if(pid2 == 0) {
        // overwrite stdin slot on FD table to the read end of the pipe
        dup2(fd[0], 0);
        // close write end of pipe
        close(fd[1]);

        // instead of taking input from stdin, this child will get input from
        // the read end of the pipe (coming from the child process)
        execvp(second_command[0], second_command);
        argError();
        exit(0);
    }
    // parent must also close their end of the pipe. 
    // This is absolutely crucial, otherwise the second child process will wait until the parent
    // is terminated to output its data.
    else {

      close(fd[0]);
      close(fd[1]);
      
      waitpid(pid, NULL, 0); // wait for 1st child to terminate
      waitpid(pid2, NULL, 0); // wait for 2nd child to terminate
      
    }
    
  }

}