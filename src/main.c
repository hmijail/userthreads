
#include <stdio.h>
#include <stdlib.h>
#include "uthreads.h"
#include <unistd.h>


uint16_t repetitions=100;

void thread_type_printer(void *param){
    //uint16_t repetitions=1000;
    char * string=(char*)param;
    //char * string="hola";
    for(uint16_t i=0;i<repetitions;i++){
        printf("%u %s ",i,string);
        usleep(300);
        mythread_yield(MTY_WHEN_QUANTUM_EXPIRES);
    }
}

void thread_type_counter(void *param){
    //uint16_t count=1000;
    uint32_t n=*(uint32_t*)param;
    //uint32_t n=12345;
    for(uint32_t i=5000;i<5000+repetitions;i++){
        printf("%u ", n++);
        usleep(300);
        mythread_yield(MTY_WHEN_QUANTUM_EXPIRES);
    }
}

void thread_type_putsA(void *param){
    for(uint16_t i=0;i<repetitions;i++){
        puts("AAAA");
        usleep(300);
        mythread_yield(MTY_WHEN_QUANTUM_EXPIRES);
    }
}

void thread_type_putsB(void *param){
    for(uint16_t i=0;i<repetitions;i++){
        puts("BBBB");
        usleep(300);
        mythread_yield(MTY_WHEN_QUANTUM_EXPIRES);
    }
}

void thread_type_blocker(void *param){
    uint16_t loops=repetitions/2;
    for(uint16_t i=loops;i>0;i--){
        printf("I will lock mutex 1 in %u\n",i);
        usleep(300);
        mythread_yield(MTY_WHEN_QUANTUM_EXPIRES);
    }
    mutex_lock(1);
    puts("LOCKED MUTEX 1");
    for(uint16_t i=loops;i>0;i--){
        printf("I will unlock mutex 1 in %u\n",i);
        usleep(300);
        mythread_yield(MTY_WHEN_QUANTUM_EXPIRES);
    }
    mutex_free(1);
    puts("FREED MUTEX 1\n");
}

void thread_type_locks_and_prints(void *param){
    for(uint16_t i=repetitions*2;i>0;i--){
        mutex_lock(1);
        printf("I managed to lock a mutex and all I got is this lousy t-shirt! (pass %u)\n",i);
        mutex_free(1);
        usleep(300);
        mythread_yield(MTY_WHEN_QUANTUM_EXPIRES);
    }
}

void thread_type_tries_to_lock_and_prints_result(void *param){
    for(uint16_t i=(uint16_t)(repetitions*1.6);i>0;i--){
        bool res=mutex_try_to_lock(1);
        printf("I could %slock the mutex (pass %u)\n",res?"":"NOT ",i);
        if(res){
            mutex_free(1);
        }
        usleep(300);
        mythread_yield(MTY_WHEN_QUANTUM_EXPIRES);
    }
}

void thread_type_reports_lock(void *param){
    for(uint16_t i=0;i<repetitions;i++){
        uint8_t free=mutex_is_free(1);
        printf("mutex is %s\n",free?"free":"locked");
        usleep(300);
        mythread_yield(MTY_WHEN_QUANTUM_EXPIRES);
    }
}

void thread_type_killer(void *param){
    thread_id victim=*(thread_id*)param;
    for(uint16_t i=(uint16_t)(repetitions*1.5);i>0;i--){
        printf("I will kill thread %u in %u\n",victim, i);
        usleep(300);
        mythread_yield(MTY_WHEN_QUANTUM_EXPIRES);
    }
    mythread_kill(victim);
}

struct philosopher_params{
    mutex_id    mutex1, mutex2;
    uint8_t     identifier;
};

void thread_type_philosopher(void *param){
    struct philosopher_params * dp=(struct philosopher_params*)param;
    mutex_id m1=dp->mutex1;
    mutex_id m2=dp->mutex2;
    for(uint16_t i=repetitions;i>0;i--){
        bool r1=mutex_try_to_lock(m1);
        if(r1){
            printf("Philosopher %u (pass %u): got mutex %u, now I want mutex %u...\n",dp->identifier, i,m1,m2);
            //usleep(100*(5+(random()%10)));
            mythread_yield(MTY_WHEN_QUANTUM_EXPIRES);
            mutex_lock(m2);
            printf("Philosopher %u (pass %u): \tgot mutexes %u and %u...\n",dp->identifier, i, m1, m2);
            usleep(100*(5+(random()%10)));
            mythread_yield(MTY_WHEN_QUANTUM_EXPIRES);
            printf("Philosopher %u (pass %u): \t\tnow I free mutexes %u and %u.\n",dp->identifier, i, m1, m2);
            mutex_free(m2);
            mutex_free(m1);
        } else {
            printf("Philosopher %u (pass %u): could not get my first mutex %u, so I wait a bit\n",dp->identifier, i, m1);
        }
        usleep(100*(5+(random()%10)));
        mythread_yield(MTY_WHEN_QUANTUM_EXPIRES);
    }
    
}

void thread_type_philosopher_smart(void *param){
    struct philosopher_params * dp=(struct philosopher_params*)param;
    mutex_id m1=dp->mutex1;
    mutex_id m2=dp->mutex2;
    for(uint16_t i=repetitions;i>0;i--){
        bool r1=mutex_try_to_lock(m1);
        //mutex_lock(m1);
        if(r1){
            printf("Smart Philosopher %u (pass %u): got mutex %u, now I want mutex %u...\n",dp->identifier, i,m1,m2);
            mythread_yield(MTY_WHEN_QUANTUM_EXPIRES);
            bool r2=mutex_try_to_lock(m2);
            if(r2){
                printf("Smart Philosopher %u (pass %u): \tgot mutexes %u and %u...\n",dp->identifier, i, m1, m2);
                usleep(100*(5+(random()%10)));
                mythread_yield(MTY_WHEN_QUANTUM_EXPIRES);
                printf("Smart Philosopher %u (pass %u): \t\tnow I free mutexes %u and %u.\n",dp->identifier, i, m1, m2);
                mutex_free(m2);
            } else {
                printf("Smart Philosopher %u (pass %u): \tcould not get my second mutex %u, so I free the first mutex %u\n",dp->identifier, i, m2, m1);
            }
            mutex_free(m1);
        } else {
            printf("Smart Philosopher %u (pass %u): could not get my first mutex %u, so I wait a bit\n",dp->identifier, i, m1);
        }
        usleep(100*(5+(random()%10)));
        mythread_yield(MTY_WHEN_QUANTUM_EXPIRES);

    }
    
}

int main(int argc, const char * argv[])
{
    uint16_t res;
    if(argc>1) {
        char * ptr;
        res=(uint16_t)strtol(argv[1],&ptr,10);
        if(*ptr==0){
            repetitions=res;
            //printf("repetitions = %u",repetitions);
        }
    }
    
    uthreads_init();
    mutexes_init();
    setbuf(stdout, NULL);
    

                                                                //THIS BLOCK TESTS KILLING AND MUTEXES
    
//    uthread_add(thread_type_tries_to_lock_and_prints_result, NULL);
//    thread_id killable=uthread_add(thread_type_locks_and_prints, NULL);
//    //uthread_add(thread_type_reports_lock, NULL);
//    uthread_add(thread_type_blocker, NULL);
//    uthread_add(thread_type_killer, (void*)&killable);
    
    
                                                                //THIS BLOCK IMPLEMENTS THE PHILOSOPHER'S DINNER
    struct philosopher_params p1={
        .mutex1=2,
        .mutex2=3,
        .identifier=1,
    };
    uthread_add(thread_type_philosopher,(void *)&p1);
    struct philosopher_params p2={
        .mutex1=3,
        .mutex2=4,
        .identifier=2,
    };
    uthread_add(thread_type_philosopher,(void *)&p2);
    struct philosopher_params p3={
        .mutex1=4,
        .mutex2=5,
        .identifier=3,
    };
    uthread_add(thread_type_philosopher,(void *)&p3);
    struct philosopher_params p4={
        .mutex1=5,
        .mutex2=2,
        .identifier=4,
    };
    uthread_add(thread_type_philosopher,(void *)&p4);

    
    uint8_t locked=uthread_scheduler();
    
    printf("Back in main. %u threads remained waiting for a mutex.\n", locked);

    return 0;
}

