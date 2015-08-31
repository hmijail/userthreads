/** @file
 *
 * An userland, no-assembler, POSIX C thread library
 *
 * Usage:
 *
 * Prepare the threads to be run by calling uthread_add() as many times as needed. In each invocation
 * give it a function pointer, which will be run in a new thread, with the given void pointer to a param.
 * What to do with the pointer is up to the function and the main program.
 * Once the threads have been added, call uthread_scheduler(). It will return when no runnable threads
 * remain; the return value is the quantity of threads locked on a mutex.
 *
 * uthread_* functions are for thread management;
 * mythread_* functions are the API callable by he threads.
 *
 * A thread is finished when its function finishes, when it calls mythread_exit() or when it is killed.
 *
 * If using collaborative threading, a thread should call periodically mythread_yield.
 *
 * Mutexes are recursive: the mutex owner can relock it. The mutex will then have to be
 * unlocked as many times as was locked.
 *
 */

#ifndef UTHREADS_H
#define UTHREADS_H

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t thread_id;
typedef uint8_t mutex_id;
typedef void (*thread_func)(void *);

enum mythread_yield_params {
    MTY_FORCED 					= 	true,	/**< yield unconditionally */
    MTY_WHEN_QUANTUM_EXPIRES 	= 	false,	/**< yield only if the quantum has been consumed */
};

///////////// SCHEDULER API /////////////////////////////////////////

/** Initialize the threads infrastructure */
void uthreads_init(void);

/** Add a thread to be run */
thread_id uthread_add(thread_func thread, void * param);

/** Run the threads that had been added
 *
 * @return					the number of threads that remained alive but unable to run because of being waiting for a mutex
 */
uint8_t uthread_scheduler(void);

///////////// THREAD API ////////////////////////////////////////////

/** Exit the current thread */
void mythread_exit(void);

/** Kill the given thread_id */
int mythread_kill(thread_id tid);

/** Yield to another thread
 *
 * @param[in]	forced		if true, yield unconditionally; if false, only yield
 * 							when the quantum has been consumed
 * @return					whether the yield actually happened
 */
bool mythread_yield(enum mythread_yield_params forced);       //needed for collaborative threads

////////////// MUTEXES //////////////////////////////////////////////

/** Initialize the mutex infrastructure */
void mutexes_init(void);

/** Is the mutex free? */
bool mutex_is_free(mutex_id id);

/** Lock the mutex; block if necessary */
void mutex_lock(mutex_id id);

/** Try to lock the mutex, fail if couldn't lock it
 *
 * If the mutex is already locked by this same thread, then it is relocked
 */
bool mutex_try_to_lock(mutex_id id);

/** Unlock the mutex */
bool mutex_free(mutex_id id);

#endif //UTHREADS_H
