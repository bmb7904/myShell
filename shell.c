/*
 * There are two ways to create a process in Linux. One is to call init(),
 * which is called at startup and it begins the kernel. The other is to fork
 * a currently existing process. The fork() function will fork the caller and
 * create a new child process. This child process is identical to the parent
 * process. The parent's entire context is cloned, including all
 * registers including its program counter, which means that the new child
 * process begins being executed at the exact same point in the machine
 * instructions as the parent when fork was called. The philosophy of UNIX
 * requires the OS to be composed of many smaller programs to carry out
 * tasks. Ir a user wants to do some task, a shell allows the user to
 * specify exactly what he/she wants, and the shell will use the small,
 * modular programs in /bin/ to carry out the tasks. We can use fork() to
 * create child processes, and then use the exec family of functions to
 * overwrite the child's memory address with a new process. I know there are
 * a lot of comments, probably too many, but I really don't want to forget my
 * thought process (no pun intended) while I was writing this.

 * Rules for my shell:

 1.) Proper spacing between commands and arguments is absolutely crucial.
 This shell is not smart enough to interpret what you are were "trying" to
 say. Improper spacing can lead to undefined behavior.

 2.) This shell has build-in commands ("cd" and "exit"). CD will change the
 working directory of the current process calling it. It takes one argument,
 which is the name of the directory to be changed to. "CD .." will change the
 working directory to that of the parent of the current working directory.

 3.) If you wish to run a process in the background, append your commands
 with a '&' character, separated by a space. This character must be the last
 character in your input line. You cannot run piped commands in the
 background, or any internal command like "cd" or "exit."
      Example: sleep 5 &

 4.) You may pipe the output of a command into another command using the '|'
 symbol, which will separate the two commands.
      Example: "cat somefile.txt | more"

 5.) You may specify to the shell that you want the output of your command to
 be redirected to a file. This is done by appending your command with the ">"
 or ">>" commands, followed by an argument (the name of the file). Using ">"
 will create a new file and write the output into the file instead of stdout.
 If a file with the specified name already exists, it will be overwritten by
 a new file with the same name. Using ">>" with a command (followed by an
 filename argument) will also create a file with the specified name and write
 the output to it. However, if a file with the specified name already exists,
 it will not be deleted, but simply appended with the redirected output.

  Example: "ls -l > sample.txt" -- creates a file sample.txt and writes the
  output of the ls -l command to it If sample.txt already exists in the
  current working directory, it is deleted and a new file of the name sample
  .txt is created and the output written to it.


   "ls -l >> sample2.txt" -- creates a file simple2.txt only if a file with
   the name sample2.txt does not already exists. If it does exist, the output
   will be appended to sample2.txt.

 6.) You may redirect input from within this shell by using the '<' character
 . This only works when running a new executable. For example: writing "./a
 .out < input.txt" IN THIS SHELL. This shell has built in functionality to
 redirect stdin to a file.

 7.) NOTE: If you wish to redirect input upon first running this shell, for
 example: "./a.out < input.txt" from a regular command line, you are using
 the linux command line's built in input redirection, not Brett's shell input
 redirection. This will still work, and this shell will read the commands in
 input.txt, but the shell will then terminate after reading commands from a
 file, as stdin in FD table for the parent process was overwritten and I
 couldn't find a way to reverse this.

 Author: Brett Bernardi

 */

#include "stdio.h"
#include "string.h"
#include <sys/types.h> // pid_t
#include <sys/wait.h>  // wait
#include <unistd.h>    // fork, execVP
#include <stdlib.h>    // exit
#include <malloc.h>    // malloc and realloc
#include <fcntl.h>
#include <errno.h>     // errno set upon functions being called

char *extractLine();
int argCounter(char *buffer);
char **tokenize(char *buffer, int numArgs);
int contains_ampersand(char **argArray);
int contains_exit(char **argArray);
void exit_program();
int getPipe(char **argArray);
void external_process(char **argArray, int bg_flag);
void argError();
void pipeProcesses(char **argArray, int pipe_index);
int contains_cd(char **argArray);
void cdCommand(char **argArray);
struct fileRedirOutput *setup_redirection(char **argArray);
void redirect_output(char **argArray, int redir_index);
void printArgArray(char **argArray);
void redirect_input(char **argArray);
struct fileRedirInput *setup_redirection_input(char **argArray);


struct fileRedirOutput {
    // will be either 0, 1, or 2 for none, ">", or ">>", respectively
    int numOfSymbols;
    // index of the redirection symbol in the argArray
    // will be set to -1 if no symbols present
    int index;
    // the filename to redirect output to
    char *filename;
};

// there will be one global struct containing information
// about output redirection into files accessable to all
// functions
struct fileRedirOutput *fr_output;

struct fileRedirInput {
    // will be either 0, 1. 0 for no redirected input symbols,
    //  or 1 for presence of "<" symbol
    int numOfSymbols;
    // index of the redirection symbol in the argArray
    // will be set to -1 if no symbols present
    int index;
    // the filename to redirect output to
    char *filename;
};

// there will be one global struct containing information
// about input redirection into files accessable to all
// functions
struct fileRedirInput *fr_input;

// the number of arguments supplied by the user. These are delineated by spaces.
// It made sense to make this global
int numArgs;


int main(int argc, char **argv) {

// Must use "\\" to print out a single "\" in printf()

    printf("--------------------------------------------------------------\n");

    printf("                         /)                      \n");
    printf("                /\\___/\\ ((                       \n");
    printf("                \\`@_@'/  ))                      \n");
    printf("                {_:Y:.}_//                       \n");
    printf("    --Brett's--{_}^-'{_}----Shell---             \n");
    printf("--------------------------------------------------------------\n\n");

    // the line of user input to be parsed and tokenized
    char *buffer;
    // an array of strings. holds each individual argument (token) input by user
    char **argArray;
    // index of the pipe in the argArray (if a pipe exits). If no pipe, set
    // to -1.
    int pipe_index = -1;


    int contains_pipe;
    // flag that will be flipped on if the '&' is contained within the user
    // argument array
    // default set to "off" or 0
    int bg_flag = 0;

    int x = 0;


    // The main loop for the shell. Only breaks out if the "exit"
    // command is entered. 
    while (1) {
        
        // the command line prompt
        write(1, "\n> ", 3);

        // get the line of user input, count and tokenize
        buffer = extractLine();
        numArgs = argCounter(buffer);
        argArray = tokenize(buffer, numArgs);

        // first check for redirected input and set up global struct
        fr_input = setup_redirection_input(argArray);
        // check for redirected output symbols and set up the global struct
        fr_output = setup_redirection(argArray);

        if (contains_exit(argArray) == 1) {
            exit_program();
        }

        if (contains_cd(argArray) == 1) {
            cdCommand(argArray);
        }
        // At this point, the user is specifiying an external command
        else {
            // get index of pipe symbol (if doesn't exit it's -1)
            pipe_index = getPipe(argArray);

            // check for bg running symbol and set flag
            bg_flag = (contains_ampersand(argArray) == 1)?  1:0;

            // add extra '\n' for external processes
            // I want spacing to be consistent
            if (fr_input->index == -1 && fr_output->index == -1) {
                write(1,"\n",1);
            }
            

            if (pipe_index != -1) {
                // user wants to pipe processes
                pipeProcesses(argArray, pipe_index);
            }
                // non piped processes
            else {
                external_process(argArray, bg_flag);
            }

        }

        free(buffer);
        free(argArray);

        // only free if memory was allocated for either the
        // struct pointer or the struct field filename char*.
        if (fr_output->filename != NULL) {
            free(fr_output->filename);
        }

        if (fr_output != NULL) {
            free(fr_output);
        }
        // only free if memory was allocated for either the
        // struct pointer or the struct field filename char*.
        if (fr_input->filename != NULL) {
            free(fr_input->filename);
        }

        if (fr_input != NULL) {
            free(fr_input);
        }

    }

    return 0;

}

/*
 * Takes the entire line of user input, stores it into a string (char*)
 * and returns the string. Will take each individual char from stdin, count
 * each char, look for the newline '\n' char. Starts with a size of 8 bytes
 * (each char is typcially 1 byte). If the number of chars from stdin exceeds
 * or equals the currently allocated char* buffer, use the realloc function.
 * Doing it this way allows for dynamic reallocation  of memory, and thus, no
 * guessing on how large user input can or will be. If an EOF is found, it
 * means that a file was used to receive input and we are at the end of the
 * file. This occurs during input redirection from Brett's shell and also
 * running Brett's shell like this: ./a.out < input.txt from a regular
 * command line (not in my shell). In either case, we exit the process after
 * getting input from file and reaching the end of the file.
 */
char *extractLine() {
    int buffer_capacity = 8;
    char *buffer;
    buffer = malloc(sizeof(char) * buffer_capacity);

    int index_into_buffer = 0;


    while (1) {
        // get a char from stdin
        // stdin is a file pointer (fp*) defined in stdio.h
        // that points to standard input stream
        char c = getc(stdin);
        // check if char is a new line (enter pressed)
        if (c == '\n') {
            // Add terminating char at end of string buffer.
            *(buffer + index_into_buffer) = '\0';
            return buffer;
        }
        // end of file reached.
        // EOF is a macro defined in stdio.h
        if (c == EOF) {
            exit(0);
        }

        *(buffer + index_into_buffer) = c;
        index_into_buffer++;


        if (index_into_buffer >= buffer_capacity) {
            buffer_capacity *= 2;
            // double capacity of buffer.
            // returns a pointer to newly allocated memory
            // Doesn't affect data already stored
            buffer = realloc(buffer, (sizeof(char) * buffer_capacity));

        }
    }
}

/*
 * Counts number of args by counting spaces in the input string buffer.
 * Returns an int.
 */
int argCounter(char *buffer) {

    int counter = 0;
    for (char *p = buffer; *(p) != EOF && *(p) != '\0'; p++) {
        if (*(p) == ' ') {
            counter++;
        }
    }

    return counter + 1;
}

/*
 * Takes the entire line of user input as a string, and tokenizes it.
 * Also takes the number of arguments contained in the string. Stores each
 * token (separated by spaces) into an array of strings. Returns the array of
 * strings (Actually an array of char* pointers).
 *
 * Preconditions: The char* buffer string in parameter must be terminated
 * with the terminating '\0' char, because the strtok() function looks for it.
 */
char **tokenize(char *buffer, int numArgs) {
    int token_index = 0;

    char **argArray;

    // Now that we know how many arguments, allocate memory for each pointer
    // in the array of char* pointers. We don't need to allocate memeory for
    // each string(inner char array) in the (outer) array, as they will remain
    // stored in the memory location they are currently in (currently pointed
    // at by char* buffer).
    argArray = malloc((sizeof(char *)) * numArgs + 1);

    /*
     * The C library function char* strtok(char *str, const char *delim)
     * breaks the string into a series of tokens using the specified delimiter.
     * This function returns a pointer to the first token found in the string.
     * A null pointer is returned if there are no tokens left to retrieve.
     * This does not create new strings in memory, it simply tokenizes the
     * each string by adding a '\0' character at the end of each token and
     * returns a pointer to each token.
     */
    char *token = strtok(buffer, " ");
    // loop through the string to extract all other tokens
    while (token != NULL) {
        *(argArray + token_index) = token;
        token = strtok(NULL, " ");

        token_index++;
    }

    // terminate the string array(char ** argArray) with a NULL pointer.
    // This is absolutely crucial. Otherwise, after multiple commands
    // entered by the user in the while loop, there will be garbage in memory
    // that will cause execvp() to not function properly. Since execvp() is
    // expecting a null pointer at the end of the array, we can simply
    // use the NULL pointer to ignore the garbage.
    argArray[token_index] = NULL;


    return argArray;
}

/*
 * Function that takes in an array of strings, searches each token for any
 * instance of the '&' character, and returns 1 if the character is found, 0
 * otherwise. If the '&' char is found, it is removed from the char** array.
 * strcmp() is used: this works by taking two pointers to char arrays as
 * arguments, and returns 0 if the two strings are equal
 */
int contains_ampersand(char **argArray) {
    for (char **p = argArray; *(p) != NULL; p++) {
        if (strcmp(*(p), "&") == 0) {
            *(p) = NULL; // take off '&' in argument array
            numArgs--; // subtract one from number of arguments.
            return 1;
        }
    }
    return 0;
}

/*
 * Function that takes in an array of strings, searches each string for any
 * redirected output symbol ">>" or ">". Creates memory for a fileRedir
 * struct defined above. If redirection symbols are found, information about
 * it is stored including the index of the symbol in the argArray. If no
 * redirection symbols found, the appropirate fields of the struct are stored.
 * Retruns a pointer to the struct.
 */
struct fileRedirOutput *setup_redirection(char **argArray) {
    int i = 0;
    // allocate memory for struct
    struct fileRedirOutput *s = malloc(sizeof(s));
    s->index = -1;

    for (char **p = argArray; *(p) != NULL; p++) {
        if (strcmp(*(p), ">") == 0) {
            s->index = i;
            s->numOfSymbols = 1;
            break;
        }
        if (strcmp(*(p), ">>") == 0) {
            s->index = i;
            s->numOfSymbols = 2;
            break;
        }
        i++;
    }
    // if no redirection symbols were presdent in argArray,
    // keep index at -1, make the rest of the fields NULL or 0,
    // and return
    if (s->index == -1) {
        s->filename = NULL;
        s->numOfSymbols = 0;
        return s;
    }

    int x = strlen(argArray[s->index + 1]);
    // alloate memory for char* (string) of fileName
    s->filename = malloc(x * sizeof(char));
    strcpy(s->filename, argArray[s->index + 1]);

    // At this point redirection symbols were found, so we remove
    //  both the symbol and filename arguments from argArray
    // by setting the char pointers in argArray to NULL. They
    // still exist in memory pointed by buffer*, but will be released
    // in another part of program. Also
    argArray[s->index] = NULL;
    argArray[s->index + 1] = NULL;
    numArgs = numArgs - 2;
    return s;

}

/*
 * Preconditions: redirection symbols were found, thus redir_index is not -1.
 * This function will take the argArray with the index of the redirection
 * symbol (redir_index), which is ">" or ">>", separate out the filename to
 * be redirected to, remove the redirection symbol from the argArray, create
 * the file if it doesn't exit, or append the file if it does exist, and
 * finally, overwrite the FD table of stdout(1) with the file descriptor of
 * the newly created file. This function should be called after forking, and
 * by a child process. As far as I know, there is no way to reverse this,
 * that is, put stdout back into FD 1.
*/
void redirect_output(char **argArray, int redir_index) {
    // checks if we are redirecting output of command
    // The following must be done after the fork to only affect child.


    if (fr_output->numOfSymbols == 1) {
        // make sure to remove redir symbol and replace with NULL pointer
        // for use later
        close(1);
        int output = open(fr_output->filename, O_CREAT | O_WRONLY | O_TRUNC,
                          0666);

        //overwrite FD of stdout
        dup2(output, 1);
    } else if (fr_output->numOfSymbols == 2) {
        close(1);
        // will create file if it does not exit and return a file pointer.
        // If file exists, will return file pointer at end of file to append
        // file. That's what "a" argument is for
        FILE *f = fopen(fr_output->filename, "a");
        // convert file pointer to file descriptor (int) using fileno() function
        int output = fileno(f);

        //overwrite FD of stdout
        dup2(output, 1);
    } else {
        // shouldn't ever get here, but just in case
        argError();
    }

}


/**
 * Function that takes in an array of strings, searches each token for any
 * instance of the "cd" command, and returns 1 if the command is found, 0
 * otherwise. strcmp() is used. Takes two pointers to char arrays(strings) as
 * arguments, and returns 0 if the two strings are equal
 */
int contains_cd(char **argArray) {
    for (char **p = argArray; *(p) != NULL; p++) {
        if (strcmp(*(p), "cd") == 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * Built-in "cd" command that will change the working directory of the
 * parent process. No forking needed. The argument at index 1 is the 
 * only argument passed to function. 
 */
void cdCommand(char **argArray) {
    if (chdir(argArray[1]) != 0) {
        perror("ERROR");
    }
}

/*
 * Will exit out of the program.
 */
void exit_program() {
    write(1, "\nGood-bye!\n\n", 12);
    exit(0);
}

/*
 * Checks if the first command was "exit".  strcmp() is used. Take two points
 * to char arrays as arguments, and returns 0 if the two
 */
int contains_exit(char **argArray) {
    for (char **p = argArray; *(p) != NULL; p++) {
        if (strcmp(*(p), "exit") == 0) {
            return 1;
        }
    }
    return 0;
    return 0;
}

/*
 * Checks array of arguments for a pipe symbol '|'. If it exits, return the
 * index of the pipe in the argArray supplied by the parameter. If no pipe
 * present, return -1.
 */
int getPipe(char **argArray) {
    int i = 0;
    for (char **p = argArray; *(p) != NULL; p++) {
        if (strcmp(*(p), "|") == 0) {
            return i;
        }
        i++;
    }

    return -1;
}

/*
 * Takes an array of strings, and the background flag, forks a child, and
 * overwrites the child's memory address with the commands in the string
 * array. If bg_flag is set to true, the parent process doesn't wait in the
 * background for the child to finish
*/
void external_process(char **argArray, int bg_flag) {
    /*
    * The execvp() takes the file name of the program you wish to use to over-
    * write the caller process as the first argument, followed by an array of
    * arguments supplied by the user to that program. This is opposed to
    * execlp(), which takes each user argument as separate arguments. Char**
    * array(array of strings) must be terminated with a NULL pointer.
    * Instead of using the relative or absolute path of the desired program,
    * members of the EXEC family of functions that contain a "p" in the name
    * (like execvp) will search for the correct path given the correct
    * program name. For example, simply using "ls" as the first argument, the
    * function searches and finds /bin/ls. execvp() only returns if there
    * was an error, and it returns a -1.
    */

    pid_t pid = fork();

    // Child process
    if (pid == 0) {

        // checks if we are redirecting output of command
        // The following must be done after the fork to only affect child.
        if (fr_output->index != -1) {

            redirect_output(argArray, fr_output->index);

        }
        // checks if there is redirected input
        // If so, set it up
        if (fr_input->index != -1) {
            redirect_input(argArray);
        }

        if (execvp(argArray[0], argArray) == -1) {
            // write to stderror which interprets the errno value
            // When a function is called in C, a variable named as errno
            // is automatically assigned a code (value) which can //be used
            // to identify the type of error that has been encountered. Its
            // a global variable indicating the error //occurred during any
            // function call and defined in the header file errno.h.
            perror("ERROR:");
            exit(1);
        }
    }
        // parent process
    else {
        // if bg_flag is false
        // don't run in background
        if (bg_flag != 1) {
            wait(NULL); // wait for child to terminate
        }

    }

}

/*
 * Writes Error message to stdout
 */
void argError() {
    write(1, "Invalid Arguments! Try again.\n", 31);
}

/*
 * Preconditions: User commands as an array of strings (char**) containing
 * two external commands separated by the '|' pipe symbol, at the specified
 * pipe_index.
 *
 * The output of the command on the left of the '|' will be piped
 * into the input of the second command. This function will separate the
 * commands on the left and right side of the pipe. The pipe will be set up
 * before any forking, and there will be two child processes needed, as
 * simply using one child and the parent will result in the shell(the parent)
 * terminating after execution. In other words, we don't want to use the
 * parent to execute any external commands(with execvp), as we can never gain
 * control back and continue with the shell. Uses the global numArgs variable.
 */
void pipeProcesses(char **argArray, int pipe_index) {
    // output goes to pipe
    char **first_command = malloc(
            sizeof(char *) * (pipe_index + 1));
    // input goes to pipe
    char **second_command = malloc(sizeof(char *) * (numArgs -
                                                     pipe_index));

    // array of file descriptors
    int fd[2];
    pid_t pid;

    for (int i = 0; i < pipe_index; i++) {
        first_command[i] = argArray[i];
    }

    *(first_command + pipe_index) = NULL;

    int k = pipe_index + 1;

    for (int j = 0; j <= (numArgs - pipe_index - 1); j++) {
        if (j == (numArgs - pipe_index - 1)) {
            second_command[j] = NULL; // add null pointer at end of array
        } else {
            second_command[j] = argArray[j + pipe_index + 1];
        }

    }

    pipe(fd); // sets up pipe in kernel space and adds file descriptors to
    // the fd[] array argument.

    pid = fork(); // fork current process

    // error checking
    if(pid < 0) {
      write(1, "Error creating process!\n", 24);
    }

    // At this point, there are two processes, and each have their own pid
    // variable. The child pid variable is set to 0, while the parent's pid
    // variable is set to the PID of the child. The child process begins
    // execution at this exact point where the fork was called, as the code
    // and PC register was copied exactly from the parent. We can use this to
    // differentiate between the two processes.

    // if true, this process is the first child.
    if (pid == 0) {
        // overwrite slot 1 (stdout) of FD table of child process with the
        // write end of the pipe. Child's process will write to pipe instead
        // of stdout
        dup2(fd[1], 1);

        // close read end of pipe
        close(fd[0]);

        if (execvp(first_command[0], first_command) == -1) {
            perror("ERROR");
            exit(1);
        }

    }
        // The parent process.
    else {

        // fork again
        pid_t pid2 = fork();

        // error checking
        if(pid2 < 0) {
        write(1, "Error creating process!\n", 24);
        }

        // The parent process now has two children, and the parent's pid and
        // pid2 variables contain the PID of its first and second child
        // processes, respectively.

        // second child
        if (pid2 == 0) {

            // after forking but before using execvp, check for
            // redirected output and call function if necessary
            if (fr_output->index != -1) {

                redirect_output(argArray, fr_output->index);

            }

            // overwrite 0 (stdin) slot on FD table to the read end of the pipe
            dup2(fd[0], 0);
            // close write end of pipe
            close(fd[1]);

            // instead of taking input from stdin, this child will get input
            // from the read end of the pipe (coming from the child process)
            if (execvp(second_command[0], second_command) == -1) {
                // write to stderror which interprets the errno value
                // When a function is called in C, a variable named as errno
                // is automatically assigned a code (value) which can be used
                // to identify the type of error that has been encountered. Its
                // a global variable indicating the error that occurred
                // during any function call and defined in the header file
                // errno.h.
                perror("ERROR");
                exit(1);
            }
        } else {
            // parent must also close their end of the pipe.
            // This is absolutely crucial, otherwise the second child process
            // will wait until the parent is terminated to output its data.
            close(fd[0]);
            close(fd[1]);

            waitpid(pid, NULL, 0); // wait for 1st child
            waitpid(pid2, NULL, 0); // wait for 2nd child

        }

    }
    // free memory from both arrays
    free(first_command);
    free(second_command);

}

/**
* Prints an array of arguments supplied by user. Used for debugging.
*/
void printArgArray(char **argArray) {
    for (char **p = argArray; *(p) != NULL; p++) {

        printf("%s ", *(p)); // take off '&' in argument array

    }

}

/*
 * Function that takes in an array of strings, searches each string for any
 * redirected input symbol "<" Creates memory for a fileRedir_input struct
 * defined above. If redirection symbol is found, information about it is
 * stored including the index of the symbol in the argArray. If no
 * redirection symbols found, the appropirate fields of the struct
 * are stored. Returns a pointer to the struct.
 */
struct fileRedirInput *setup_redirection_input(char **argArray) {
    int i = 0;
    // allocate memory for struct
    struct fileRedirInput *s = malloc(sizeof(s));
    s->index = -1;

    for (char **p = argArray; *(p) != NULL; p++) {
        if (strcmp(*(p), "<") == 0) {
            s->index = i;
            s->numOfSymbols = 1;
            break;
        }
        i++;
    }
    // if no redirection symbols were presdent in argArray,
    // keep index at -1, make the rest of the fields NULL or 0,
    // and return
    if (s->index == -1) {
        s->filename = NULL;
        s->numOfSymbols = 0;
        return s;
    }

    int x = strlen(argArray[s->index + 1]);
    // alloate memory for char* (string) of fileName
    s->filename = malloc(x * sizeof(char));
    strcpy(s->filename, argArray[s->index + 1]);

    // At this point redirection symbols were found, so we remove
    //  both the symbol and filename arguments from argArray
    // by setting the char pointers in argArray to NULL. They
    // still exist in memory pointed by buffer*, but will be released
    // in another part of program
    argArray[s->index] = NULL;
    argArray[s->index + 1] = NULL;
    return s;

}

/*
 * Preconditions: redirect input symbol was found, thus redir_index is not -1.
 * This function will take the argArray with the index of the redirection
 * symbol (redir_index), which is "<", separate out the filename to be
 * redirected to, remove the redirection symbol from the argArray,open the
 * file,and overwrite the FD table of stdin(0) with the file descriptor of
 * the newly opened file. This function should be called by the child.
*/
void redirect_input(char **argArray) {

    if (fr_input->numOfSymbols == 1) {
        // make sure to remove redir symbol and replace with NULL pointer
        // for use later
        close(0);
        int input = open(fr_input->filename, 0);
        // open the file supplied in redirected input command
        if (input != -1) {
            // replace stdin in FD table with file descriptor from opened file
            dup2(input, 0);
        } else {
            perror("ERROR OPENING FILE");
            exit(1);
        }


    } else {
        // shouldn't ever get here, but just in case
        argError();
    }

}