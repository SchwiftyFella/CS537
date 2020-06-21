#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sched.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <limits.h>

void setCPU()
{
    cpu_set_t myCPUs;
    //CPU_ZERO(&myCPUs);
    //sched_getaffinity(0,sizeof(cpu_set_t),&myCPUs);
    //int p = CPU_COUNT(&myCPUs);
    CPU_ZERO(&myCPUs);
    CPU_SET(0,&myCPUs);
    if( sched_setaffinity(0,sizeof(cpu_set_t),&myCPUs) == -1 )
        perror("cant change the cpu affinity of the process:");
    else{
        sched_getaffinity(0,sizeof(cpu_set_t),&myCPUs);
        int p = CPU_COUNT(&myCPUs);
        printf("Num CPUs in set %d\n", p);
    }
    printf("The scheduling priority of the process is %d\n", sched_getscheduler(0));
}

int main()
{
    setCPU();
    int i = 1;
    while( i < 20 ){
        if( fork() == 0 ){
            setpriority(PRIO_PROCESS,0,i);
            break;
        }
        i++;
    }
    
    i = 0;
    while( i <= 0.9*INT_MAX )
        i++;
        
    int p = getpriority(PRIO_PROCESS,0);
    printf("priority %d\n", p);
    
    while( wait(NULL) != -1);
    return 0;
}