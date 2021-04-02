/*
 * Shell
 */


#include "stdio.h"
#include "string.h"
#include <sys/types.h> // pid_t
#include <sys/wait.h>  // wait
#include <unistd.h>    // fork, execlp
#include <stdlib.h>    // exit

char* extractLine();
int argCounter(char* buffer);
char** tokenize(char* buffer, int numArgs);
int contains_ampersand(char** argArray, int numArgs);


int numArgs;


int main(int argc, char** argv) {
    printf("Hello and welcome to Brett Bernardi's shell!\n\n");

    // the line of input to be parsed and tokenized
    char *buffer;
    // an array of strings. holds each individual argument input by user
    char **argArray;

    printf(">> ");

    buffer = extractLine();

    numArgs = argCounter(buffer);

    printf(" numArgs = %d and lol: %s\n", numArgs, buffer);

    argArray = tokenize(buffer, numArgs);

    printf("\n List of args: ");
    for(int i = 0 ; i < numArgs; i++)
    {

        printf("%s ", argArray[i]);
    }


    if(contains_ampersand(argArray, numArgs) == 1) {
        printf("\nIt contains an &!\n");
    }
    else {
        printf("\nThere is no ampersand!\n");
    }


    /*
     * The execvp() takes the file name of the program you wish overwrite
     * the current process with to as the first argument, followed by an
     * array of the arguments supplied to that program. This is opposed to
     * execlp(), which takes each argument as separate arguments. As long as
     * your array is terminated with a null pointer. Instead of using the
     * relative and absolute path, members of this family of functions that
     * contain a "p" will search for the correct path given the correct
     * program name. For example, simply using "ls" as the first argument,
     * the function search and find /bin/ls.
     */
    pid_t pid = fork();

    if(pid == 0) {
        execvp(argArray[0], argArray);
    }
    else {
        wait(NULL);
    }

    return 0;
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
    int size_buffer = 40;
    char *buffer;
    buffer = malloc(sizeof(char) * size_buffer);

    int index_into_buffer = 0;


    while(1) {
        char c = getchar();
        if (c == EOF || c == '\n') {
            buffer[index_into_buffer] = '\0';
            return buffer;
        }

        buffer[index_into_buffer] = c;
        index_into_buffer++;

        if (index_into_buffer >= size_buffer) {
            size_buffer = size_buffer * 2;
            // increase size of buffer.
            buffer = realloc(buffer, size_buffer);
        }
    }
}

/*
 * Counts number of args by counting spaces in a string before tokenization.
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
 * Tokenize the incoming line and store each token (separated by spaces) into
 * an array of strings. Return the array of strings
 */
char** tokenize(char* buffer, int numArgs) {
    int numTokens = 0;

    char** argArray;

    // Now that we know how many arguments, allocate memory for each pointer
    // in the array of char* pointers. Dont need to allocate memeory for each
    // string in the array, as they will remain stored in the memory location
    // they are currently in (currently pointed at by char* buffer).
    argArray = malloc((sizeof(char*)) * numArgs);

    /*
     * The C library function char *strtok(char *str, const char *delim)
     * breaks string str into a series of tokens using the specified delimiter .
     * This function returns a pointer to the first token found in the
     * string. A null pointer is returned if there are no tokens left to retrieve.
     * This does not create new strings, it simply returns pointers to tokens
     *
     */
    char *token = strtok(buffer, " ");
    // loop through the string to extract all other tokens
    while( token != NULL ) {
        argArray[numTokens] = token;
        token = strtok(NULL, " ");

        numTokens++;
    }


    return argArray;
}

/**
 * Function that takes in an array of strings, searches each token for any
 * instance of the '&' character, and returns 1 if the character is found, 0
 * otherwise.
 */
int contains_ampersand(char** argArray, int numArgs) {
    for(int i = 0; i < numArgs; i++) {
        if(*(argArray[i]) == '&') {
            return 1;
        }
    }
    return 0;
}