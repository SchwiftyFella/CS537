#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

sem_t gryffindorQueue, slytherinQueue, ravenPuffQueue, seatLock;
int griffs_InCar, slyths_InCar, ravenPuffs_InCar;
int griffs_remaining, slyth_remaining, ravenPuff_remaining;

typedef struct {
    int total;
    int current;
    sem_t mutex;
    sem_t turnstile;
    sem_t turnstile2;
} barrier_t;

void barrier_init(barrier_t *barrier, int total){
    barrier->total = total;
    barrier->current = 0;
    sem_init(&barrier->mutex, 0, 1);
    sem_init(&barrier->turnstile, 0, 0);
    sem_init(&barrier->turnstile2, 0, 0);
}

void barrier_wait(barrier_t *barrier)
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
/* ---------------- end of barrier class ---------------- */

barrier_t barrier;

void boardCar(char* house, int id)
{
    printf("Student %s%d boarded\n", house, id);
    
    barrier_wait(&barrier);
}

void captains_report(){
    sem_wait(&seatLock);
    printf("**The car has left the station**\n");
    griffs_InCar = slyths_InCar = ravenPuffs_InCar = 0;
    if(griffs_remaining > 0)
        sem_post(&gryffindorQueue);
    else if(slyth_remaining > 0)
        sem_post(&slytherinQueue);
    else
        sem_post(&ravenPuffQueue);
    sem_post(&seatLock);
}

void *griffSpell(void *arg)
{
    bool isCaptain = false;
    int id = *(int*)arg;
    
    sem_wait(&gryffindorQueue);

    sem_wait(&seatLock);

    if (griffs_InCar + slyths_InCar + ravenPuffs_InCar == 0)
    {
        isCaptain = true;                   // the thread becomes the captain

        if(griffs_remaining > 3)//we havent subtracted ourselves yet
        {
            //relase 3
            griffs_InCar +=1;
            griffs_remaining--;
            sem_post(&gryffindorQueue);
            sem_post(&gryffindorQueue);
            sem_post(&gryffindorQueue);
        }
        else if(griffs_remaining == 3 && ravenPuff_remaining >= 1){
            griffs_InCar +=1;
            griffs_remaining--;
            sem_post(&gryffindorQueue);
            sem_post(&gryffindorQueue);
            sem_post(&ravenPuffQueue);
        }
        else if(griffs_remaining == 2 && ravenPuff_remaining >= 2){
            griffs_InCar +=1;
            griffs_remaining--;
            sem_post(&gryffindorQueue);
            sem_post(&ravenPuffQueue);
            sem_post(&ravenPuffQueue);
        }
        else if(griffs_remaining == 1 && slyth_remaining >= 1 && ravenPuff_remaining >= 2){
            griffs_InCar +=1;
            griffs_remaining--;
            sem_post(&slytherinQueue);
            sem_post(&ravenPuffQueue);
            sem_post(&ravenPuffQueue);
        }
        else if(griffs_remaining == 1 && ravenPuff_remaining >= 3){
            griffs_InCar +=1;
            griffs_remaining--;
            sem_post(&ravenPuffQueue);
            sem_post(&ravenPuffQueue);
            sem_post(&ravenPuffQueue);
        }
        else{
            printf("Griff%d has been left behind. No suitable combination of compatible students\n", id);
            griffs_remaining--;

            if(griffs_remaining > 0)
                sem_post(&gryffindorQueue);
            else if(slyth_remaining > 0)
                sem_post(&slytherinQueue);
            else
                sem_post(&ravenPuffQueue);

            sem_post(&seatLock);
            return NULL;
        }

    }
    else
    {
        griffs_InCar +=1;
        griffs_remaining--;
    }

    sem_post(&seatLock);
    boardCar("Griff", id);

    if (isCaptain)
        captains_report();
}

void *slythSpell(void *arg)
{
    bool isCaptain = false;
    int id = *(int*)arg;
    
    sem_wait(&slytherinQueue);

    sem_wait(&seatLock);

    if (griffs_InCar + slyths_InCar + ravenPuffs_InCar == 0)
    {
        isCaptain = true;                   // the thread becomes the captain

        if(slyth_remaining > 3)//we havent subtracted ourselves yet
        {
            //relase 3
            slyths_InCar +=1;
            slyth_remaining--;
            sem_post(&slytherinQueue);
            sem_post(&slytherinQueue);
            sem_post(&slytherinQueue);
        }
        else if(slyth_remaining == 3 && ravenPuff_remaining >= 1){
            slyths_InCar +=1;
            slyth_remaining--;
            sem_post(&slytherinQueue);
            sem_post(&slytherinQueue);
            sem_post(&ravenPuffQueue);
        }
        else if(slyth_remaining == 2 && ravenPuff_remaining >= 2){
            slyths_InCar +=1;
            slyth_remaining--;
            sem_post(&slytherinQueue);
            sem_post(&ravenPuffQueue);
            sem_post(&ravenPuffQueue);
        }
        else if(slyth_remaining == 1 && ravenPuff_remaining >= 3){
            slyths_InCar +=1;
            slyth_remaining--;
            sem_post(&ravenPuffQueue);
            sem_post(&ravenPuffQueue);
            sem_post(&ravenPuffQueue);
        }
        else{
            printf("Slyth%d has been left behind. No suitable combination of compatible students\n", id);
            slyth_remaining--;

            if(slyth_remaining > 0)
                sem_post(&slytherinQueue);
            else
                sem_post(&ravenPuffQueue);

            sem_post(&seatLock);
            return NULL;
        }

    }
    else
    {
        slyths_InCar +=1;
        slyth_remaining--;
    }

    sem_post(&seatLock);
    boardCar("Slyth", id);

    if (isCaptain)
        captains_report();
}

void *ravenSpell(void *arg)
{
    bool isCaptain = false;
    int id = *(int*)arg;
    
    sem_wait(&ravenPuffQueue);

    sem_wait(&seatLock);

    if (griffs_InCar + slyths_InCar + ravenPuffs_InCar == 0)
    {
        isCaptain = true;                   // the thread becomes the captain

        if(ravenPuff_remaining > 3)//we havent subtracted ourselves yet
        {
            //relase 3
            ravenPuffs_InCar +=1;
            ravenPuff_remaining--;
            sem_post(&ravenPuffQueue);
            sem_post(&ravenPuffQueue);
            sem_post(&ravenPuffQueue);
        }
        else{
            printf("Raven%d has been left behind. No suitable combination of compatible students\n", id);
            ravenPuff_remaining--;

            sem_post(&ravenPuffQueue);

            sem_post(&seatLock);
            return NULL;
        }

    }
    else
    {
        ravenPuffs_InCar +=1;
        ravenPuff_remaining--;
    }

    sem_post(&seatLock);
    boardCar("Raven", id);

    if (isCaptain)
        captains_report();
}

int main(void)
{
    printf("Enter num of Gryffindor students: \n");
    scanf("%i", &griffs_remaining);
    printf("Enter num of Slitherin students: \n");
    scanf("%i", &slyth_remaining);
    printf("Enter num of Raven/Puff students: \n");
    scanf("%i", &ravenPuff_remaining);

    // create and initialize our barrier
    barrier_init(&barrier, 4);

    // initialize our sems
    sem_init(&gryffindorQueue, 0, 0);
    sem_init(&slytherinQueue, 0, 0);
    sem_init(&ravenPuffQueue, 0, 0);
    sem_init(&seatLock, 0, 0);

    // initialize our shared ints
    griffs_InCar = slyths_InCar = ravenPuffs_InCar = 0;

    int n = griffs_remaining + slyth_remaining + ravenPuff_remaining;
    pthread_t threads[n];
    int id[n];

    // create threads
    for (int i = 0; i < n; i++)
    {
        if (i < griffs_remaining)
        {
            id[i] = i;
            pthread_create(&threads[i], NULL, griffSpell, &id[i]);
            printf("Created Gryffindor student: Griff%d\n", i);
        }
        else if (i < griffs_remaining + slyth_remaining)
        {
            id[i] = (i - griffs_remaining);
            pthread_create(&threads[i], NULL, slythSpell, &id[i]);
            printf("Created Slytherin student: Slyth%d\n",(i - griffs_remaining));
        }
        else
        {
            id[i] = (i - griffs_remaining - slyth_remaining);
            pthread_create(&threads[i], NULL, ravenSpell, &id[i]);
            printf("Created Raven/Puff student: Raven%d\n", (i - griffs_remaining - slyth_remaining));
        }
    }

    //find a captain
    if(griffs_remaining > 0)
        sem_post(&gryffindorQueue);
    else if(slyth_remaining > 0)
        sem_post(&slytherinQueue);
    else
        sem_post(&ravenPuffQueue);

    sem_post(&seatLock);
    
    for (int i = 0; i < n; i++)
    {
        // create pthread with function task and pass in the barrier as the arg
        pthread_join(threads[i], NULL);
    }
    return 0;
}