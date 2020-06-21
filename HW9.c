#include <stdio.h>
#include <pthread.h>
#include <limits.h>
#include <semaphore.h>
#include <signal.h>


typedef struct {
    volatile sig_atomic_t counter;
    sem_t mutex;
} semSlim_t;

void semSlim_init(semSlim_t *sem)
{
    sem->counter = 1; //set to 1 so threads are not locked out
    sem_init(&sem->mutex, 0, 1);
}

void semSlim_wait(semSlim_t *sem) {
    for (; ;) {
        // Spin, waiting until we see a positive counter
        while (sem->counter <= 0) { }

        sem_wait(&sem->mutex);
        
        // Double check if counter is still positive this time in a mutex
        if (sem->counter <= 0) {
            sem_post(&sem->mutex);
            continue;
        }

        //If we get through lower the counter so we can block other threads from checking
        sem->counter--;
        sem_post(&sem->mutex);
        break;
    }
}

void semSlim_signal(semSlim_t *sem) {
    sem_wait(&sem->mutex);
    sem->counter++;
    sem_post(&sem->mutex);
}


static semSlim_t sem;
static volatile sig_atomic_t work = 0;

void* thread_work(void* args) {

    printf("Worker: %d ready to contribute\n", *(int*)args);
    
    for (; ; ) {
        semSlim_wait(&sem);

        if(work < 0.01 * INT_MAX)
        {
            work += 100;
            printf("Worker: %d, Current Work: %d\n", *(int*)args, work);
            semSlim_signal(&sem);
        }
        else{
            printf("Worker: %d completed\n", *(int*)args);
            semSlim_signal(&sem);
            break;
        }
    }

    return NULL;
}


int main() {
    pthread_t threads[5];
    int id[5];
    for (int i = 0; i < 5; ++i) {
        id[i] = i + 1;
    }
    semSlim_init(&sem);
    for (int i = 0; i < 5; ++i) {
        pthread_create(&threads[i], NULL, thread_work, &id[i]);
    }

    for (int i = 0; i < 5; ++i) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}