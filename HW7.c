#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>
#include <semaphore.h>

pthread_t threads[5] = {};
int completedThreads;                   

typedef struct {
    int total;
    int current;
    sem_t mutex;
    sem_t turnstile;
    sem_t turnstile2;
} barrier_t;

void lock(barrier_t *barrier)
{
    sem_wait(&barrier->mutex);
    if (++barrier->current == barrier->total)
    {
        for (int i = 0; i < barrier->total; i++)
        {
            sem_post(&barrier->turnstile);
        }
    }
    sem_post(&barrier->mutex);
    sem_wait(&barrier->turnstile);

    sem_wait(&barrier->mutex);
    if(--barrier->current == 0)
    {
        for (int i = 0; i < barrier->total; i++)
        {
            sem_post(&barrier->turnstile2);
        }
    }
    sem_post(&barrier->mutex);
    sem_wait(&barrier->turnstile2);
}

void *task(void *arg)
{
    barrier_t *barrier = arg;

    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 0.1 * INT_MAX; j++)
        {
        }
        
        printf("Thread %li on iteration %i\n", pthread_self(), i);
        lock(barrier);
    }

    if (pthread_detach(pthread_self()) == -1)
        printf("Error while detaching thread: %li\n", pthread_self());

    completedThreads += 1;
}

int main(void)
{
    completedThreads = 0;

    // create the barrier
    barrier_t barrier;

    barrier.total = 5;
    barrier.current = 0;
    sem_init(&barrier.mutex, 0, 1);
    sem_init(&barrier.turnstile, 0, 0);
    sem_init(&barrier.turnstile2, 0, 0);

    // create threads
    for (int i = 0; i < 5; i++)
    {
        // create pthread and assign task and pass in the barrier as the arg
        if (pthread_create(&threads[i], NULL, task, &barrier) == -1)
            printf("Failed to create thread %li\n", threads[i]);
        else
            printf("Created thread %i: %li\n", (i + 1), threads[i]);
    }

    for(;;)
    {
        sleep(1);

        // if all theads have completed we exit
        if (completedThreads >= 4)
            exit(0);
    }
}