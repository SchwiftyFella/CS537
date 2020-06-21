#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h> 
#include <sys/time.h> 
#include <stdbool.h>

static volatile int signalCount = 0;

void handle_sigint(int sig) 
{ 
    signalCount++;
    puts("");
    printf("signal count: %d\n", signalCount); 
} 

void empty_signal(int sig) 
{ 

}

void trackTime(){

    struct timespec start, current;

    clock_gettime(CLOCK_REALTIME, &start);
    bool oneHundredHit = false;

    do{
        clock_gettime(CLOCK_REALTIME, &current);

        if(!oneHundredHit && current.tv_sec - start.tv_sec >= 100){
            //signal to ignore
            signal(SIGINT, empty_signal);//modify the handler

            //print the hellos
            for (int i = 0; i < signalCount; i++)
            {
                puts("Hello");
            }

            oneHundredHit = true;
        }

    }while (current.tv_sec - start.tv_sec < 200);   
}

  
int main() 
{

    if (fork() == 0) //child thread tracks SIGINT and then prints the output when appropriate
    {
        signal(SIGINT, handle_sigint); 
        trackTime();
    }
    else{ //main thread waits for child and ignores SIGINT
        signal(SIGINT, empty_signal);
    }

    wait(NULL);
    return 0; 
} 