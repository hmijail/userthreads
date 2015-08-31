

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include "uthreads.h"
#include <string.h>
#include <alloca.h>
#include "config.h"
#include <assert.h>

enum general_constants{
    num_user_threads        = 10,
    num_mutexes             = 255,
    ut_quantum              = 1,             //ms
    stack_rsrv_per_thread   = SIGSTKSZ
};

typedef sig_atomic_t mutex_owner;

struct user_thread_st {
    thread_func         func;
    uint8_t volatile    status;                     // r/w in signal handler and scheduler
    sigjmp_buf          environ;
    void*               params;
    mutex_id            mutex_waited;
};

struct user_thread_st uthreads[num_user_threads];   // could be implemented as some list for greater flexibility

enum thread_ids {
    THREAD_NONE=255
};

/** used in the longjmp from thread to scheduler */
enum user_thread_status {
    UTS_NOT_RUN     =	1,
    UTS_DEAD        =	2,
    UTS_PREEMPTED   =	3,
    UTS_LOCK_WAIT   =	4
};

/** used in the longjmp from scheduler to thread */
enum scheduler_status {
    SS_CONTINUE     =	1,
    SS_LOCK_SECURED =	2
};

sigjmp_buf scheduler_environ;

uint8_t ut_current;
sig_atomic_t volatile ut_quantum_finished;   // volatile because written in signal handler, read in thread


struct mutex_st {
    mutex_owner     owner;          //!< OWNER_NONE if free mutex, else uthread index
    uint8_t         count;          //!< for a recursive mutex
};

struct mutex_st mutexes[num_mutexes];

enum mutex_values {
    OWNER_NONE = THREAD_NONE,
    MUTEX_NONE = num_mutexes
};

void uthreads_init(void) {
	for (thread_id i = 0; i < num_user_threads; i++) {
		uthreads[i].status = UTS_DEAD;
		uthreads[i].mutex_waited = MUTEX_NONE;
	}
}

void mutexes_init(void) {
	for (uint8_t i = 0; i < num_mutexes; i++) {
		mutexes[i].owner = OWNER_NONE;
	}
}

sigset_t sigmask_interrupt; //initialized in the scheduler to only contain "our" signal
enum interrupt_parms {
	signal_interrupt = SIGALRM, itimer_mode = ITIMER_REAL
};

static void signal_block(void) {
	sigprocmask(SIG_BLOCK, &sigmask_interrupt, NULL);
}

static void signal_unblock(void) {
	sigprocmask(SIG_UNBLOCK, &sigmask_interrupt, NULL);
}

/** schedule a single SIGVTALRM in ut_quantum ms */
static int signal_schedule(void) {
	struct itimerval const itv = {
		.it_interval = { 0 },
		.it_value = {
			.tv_sec = 0,
			.tv_usec = ut_quantum * 1000,
		}
	};
	int res = setitimer(itimer_mode, &itv, NULL);
	return res;
}

static void signal_cancel(void) {                       //cancel any pending SIGVTALRM
	struct itimerval const itv_zero = {
		.it_interval = { 0 },
	    .it_value = { 0 },
	};
	setitimer(itimer_mode, &itv_zero, NULL);
}

bool mutex_is_free(mutex_id id) {
	return mutexes[id].owner == OWNER_NONE;
}

void mutex_lock(mutex_id id) {
	struct mutex_st * const m = &mutexes[id];     	//shortcut for clarity
	if (m->owner == ut_current) {                   //recursive mutex
		m->count++;                             	//TODO: might overflow
		assert(m != 0);
		return;
	}
	signal_block();
	if (mutex_is_free(id)) { //might be better done using GCC's builtin __sync_bool_compare_and_swap
		m->owner = ut_current;
		//assert(mutexes[id].count==0)
		m->count = 1;
	} else {
		uthreads[ut_current].mutex_waited = id;
		if (!sigsetjmp(uthreads[ut_current].environ, true)) {
			uthreads[ut_current].status = UTS_LOCK_WAIT;
			siglongjmp(scheduler_environ, UTS_LOCK_WAIT);
		}
		//will only return here when the mutex has been successfully locked
	}
	signal_unblock();
	return;
}

bool mutex_try_to_lock(mutex_id id) {
	struct mutex_st *m = &mutexes[id];				//shortcut for clarity
	bool result;
	signal_block();
	if ((!mutex_is_free(id)) && (m->owner != ut_current)) {
		result = false;
	} else {
		mutex_lock(id);
		result = true;
	}
	signal_unblock();
	return result;
}

bool mutex_free(mutex_id id) { 						//returns true if the freeing had effect; false if error
	struct mutex_st *m = &mutexes[id];
	if (m->owner == ut_current) {
		m->count--;
		if (m->count == 0) { 						//...could be much more compact, but I don't like obfuscation ;P
			m->owner = OWNER_NONE;
		}
		return true;
	}
	return false;
}

static bool uthread_yield(enum mythread_yield_params forced) {          //returns true if it actually yielded
	bool result = false;
	if (forced || ut_quantum_finished) {
		if (CONFIG_VERBOSE)
			printf("YIELDING in thread %u\n", ut_current);
		if (forced) {
			signal_cancel();
		}
		if (!sigsetjmp(uthreads[ut_current].environ, true)) {
			uthreads[ut_current].status = UTS_PREEMPTED;
			siglongjmp(scheduler_environ, UTS_PREEMPTED);
		}
		result = true;
		if (CONFIG_VERBOSE)
			printf("RETURNED to thread %u\n", ut_current);
	}
	return result;
}

static void timer_catcher(int signum) {

	if (CONFIG_PREEMPTIVE_SCHEDULING) {
		mythread_yield(true);
	} else {
		ut_quantum_finished = true;
	}
}

bool mythread_yield(enum mythread_yield_params forced) {
	if (CONFIG_PREEMPTIVE_SCHEDULING && !forced)
		return false;
	return uthread_yield(forced);
}

void mythread_exit(void) {
	signal_cancel();
	uthreads[ut_current].status = UTS_DEAD;
	siglongjmp(scheduler_environ, UTS_DEAD);
}

int mythread_kill(thread_id tid) {
	if ((tid >= num_user_threads) || (tid < 1)) {               //sanitize param
		return -1;
	}
	uthreads[tid].status = UTS_DEAD;
	//XXX might be interesting to clean up the killed thread's locks
	return 0;
}

thread_id uthread_add(thread_func thread, void * param) {
	for (thread_id i = 0; i < num_user_threads; i++) {
		if (uthreads[i].status != UTS_DEAD) {
			continue;
		}
		uthreads[i].status = UTS_NOT_RUN;
		uthreads[i].func = thread;
		uthreads[i].params = param;
		return i;
	}
	return THREAD_NONE;
}

uint8_t ut_visited;
uint8_t uthreads_locked_count;

static void ut_scheduler_watchdog_reset(void) { //take note that the scheduler still has something to run
	ut_visited = 0;
	uthreads_locked_count = 0;
	if (CONFIG_VERBOSE)
		printf("WATCHDOG RESET by thread %u\n", ut_current);
}

uint8_t uthread_scheduler(void) {
	//init auxiliary vars
	sigemptyset(&sigmask_interrupt);
	sigaddset(&sigmask_interrupt, signal_interrupt);

	//save old signal action, set new action
	struct sigaction action_old, action_new;
	action_new.sa_handler = timer_catcher;
	sigemptyset(&action_new.sa_mask);
	action_new.sa_flags = SA_RESTART;
	sigaction(signal_interrupt, &action_new, &action_old);

	//save old signal mask, set new
	sigset_t sigmask_old;
	sigprocmask(SIG_UNBLOCK, &sigmask_interrupt, &sigmask_old);

	//start scheduling "loop"
	if (sigsetjmp(scheduler_environ, true)) {
		ut_current++;
	}
	if (CONFIG_VERBOSE)
		printf("SCHEDULER, will try now thread %u\n", ut_current);

	//find next runnable thread, or return if none

	while ((uthreads[ut_current].status == UTS_DEAD)
	        && !(ut_visited == num_user_threads)) {
		ut_visited++;
		if (CONFIG_VERBOSE)
			printf("current=%u, DEAD, visited=%u\n", ut_current, ut_visited);
		ut_current++;
		if (ut_current == num_user_threads) {
			ut_current = 0;
		}
	}

	if (ut_visited == num_user_threads) { //no threads were runnable (all were dead or waiting for a mutex)
		sigaction(signal_interrupt, &action_old, NULL);
		sigprocmask(SIG_SETMASK, &sigmask_old, NULL);
		return uthreads_locked_count;
	}

	//at this point, ut_current points to a potentially runnable thread

	ut_visited++;
	ut_quantum_finished = false;
	signal_schedule();

	switch (uthreads[ut_current].status) {
	case UTS_PREEMPTED:
		ut_scheduler_watchdog_reset();
		siglongjmp(uthreads[ut_current].environ, SS_CONTINUE);
		break;
	case UTS_NOT_RUN: {
		uint32_t reservation = (ut_current + 1) * stack_rsrv_per_thread;
		//void volatile * resrv=alloca(reservation);
		//*(uint8_t*)resrv=0;
		uint8_t volatile __attribute__((unused)) stack_reserve[reservation];
		stack_reserve[0] = 0; //avoids the compiler optimizing out the reservation
		ut_scheduler_watchdog_reset();
		uthreads[ut_current].func(uthreads[ut_current].params);
		//if the function returns, force an explicit exit
		if (CONFIG_VERBOSE)
			printf("ENDED thread %u\n", ut_current);
		mythread_exit();
		break;
	}
	case UTS_LOCK_WAIT: {
		mutex_id m_waited = uthreads[ut_current].mutex_waited;
		if (mutex_is_free(m_waited)) {
			mutex_lock(m_waited);
			ut_scheduler_watchdog_reset();
			siglongjmp(uthreads[ut_current].environ, SS_LOCK_SECURED);
		} else {                      //we could implement "robust mutexes" here
			signal_cancel();
			uthreads_locked_count++;
			if (CONFIG_VERBOSE)
				printf("THREAD %u LOCKED, count=%u, visited=%u\n", ut_current,
				        uthreads_locked_count, ut_visited);
			siglongjmp(scheduler_environ, UTS_LOCK_WAIT);
		}
		break;
	}

	}
	//we should never reach this point
	__builtin_unreachable();
}
