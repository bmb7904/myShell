/*
 * Creates a sequence of worker threads to find large primes and joins 
 * the workers to the main thread. This version has a bug for the sake 
 * of illustration. 
 *
 * Author: Drue Coles
 */

#include <pthread.h>
#include <unistd.h>
#include <stdio.h> 
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define NUM_THREADS 5
int args[NUM_THREADS];

void* get_prime(void*); 
int is_prime(int);

int main() {
    pthread_t threads[NUM_THREADS];
    srand(time(0));
    
    pthread_attr_t attr; // set thread detached attribute
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for (int t = 0; t < NUM_THREADS; t++) {
        printf("Creating thread %d.\n", t);
        args[t] = t;
        pthread_create(&threads[t], &attr, get_prime, (void *) &args[t]);
        // Why not join each thread here?
    }
    pthread_attr_destroy(&attr);  // Or else: memory leak.
    
    // Join the workers to the main thread.
    int* result;
    for (int t = 0; t < NUM_THREADS; t++) { 
        pthread_join(threads[t], (void*) &result);
        printf("Main: thread %d returns %d\n", t, *result);
    }

    return 0;
}

// problem here.
void* get_prime(void* t) {   
    int arg = *((int*) t);
    int max = (int) pow(10, 4 + arg);
    int ret_val = rand() % max;
    while (!is_prime(ret_val))
        ret_val = rand() % max;

    sleep(rand() % 3); 
    printf("Thread %d done.\n", arg);
    pthread_exit((void*) &ret_val);
}

int is_prime(int n) {
    for (int i = 2; i < n; i++)
        if (n % i == 0)
            return 0;  
    return 1; 
}