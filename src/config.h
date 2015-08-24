

#ifndef Userland_threads_config_h
#define Userland_threads_config_h

//Preemptive scheduling causes a thread to be switched out (using longjmp) from the signal handler; so a function doesn't have to do anything special to work as a thread.
//But that has some problems:
//  longjmp is not an an async-safe function, so is not supposed to be called from a signal handler;
//  using longjmp from a signal handler apparently causes the kernel to consider the rest of program execution to be still inside of a signal handler, which can have an effect on the delivery of other signals
//  CERT considers (sig)longjmp usage inside of a signal handler to be a vulnerability
//    ( https://www.securecoding.cert.org/confluence/display/seccode/SIG30-C.+Call+only+asynchronous-safe+functions+within+signal+handlers )
//An alternative is using collaborative/non-preemptive scheduling. In this mode, the signal handler only marks the expiry of the thread's quantum, and the thread is required to call periodically the mythread_yield(bool forced) function. This function will only actually yield if called with forced=true, or if the thread's time quantum has expired. If using preemptive scheduling, mythread_yield(false) will simply return.
//The preemptive option sometimes segfaults in Ubuntu on a VM, but seemingly never segfaults in Mac OS X.

//#define CONFIG_PREEMPTIVE_SCHEDULING   true
#define CONFIG_PREEMPTIVE_SCHEDULING   false


//This logs the most important checkpoints of the program
#define CONFIG_VERBOSE                 false
//#define CONFIG_VERBOSE                 true

#endif
