

#ifndef uthreads_h
#define uthreads_h

#include <stdbool.h>
#include <stdint.h>
#include "config.h"

typedef uint8_t thread_id;
typedef uint8_t mutex_id;
typedef void (*thread_func)(void *);

void mutexes_init(void);
void uthreads_init(void);

bool mutex_is_free(mutex_id id);
void mutex_lock(mutex_id id);
bool mutex_try_to_lock(mutex_id id);
bool mutex_free(mutex_id id);


thread_id uthread_add(thread_func thread, void * param);

int mythread_exit(void);
int mythread_kill(int tid);

bool mythread_yield(bool forced);       //needed for non-preemptive threads

enum mythread_yield_params {
    MTY_FORCED = true,
    MTY_WHEN_QUANTUM_EXPIRES = false,
};

uint8_t uthread_scheduler(void);

#endif
