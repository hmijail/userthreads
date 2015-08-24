# C/POSIX portable userland threads

This is a microproject/proof-of-concept that I made some time ago. It needs some cleaning up and polishing, but it's already working, so good enough for a first public commit.

I have seen other implementations, and it's interesting how different can they be. So in the following I document my design choices. 

But first of all, the non-choice: this was made when I was transitioning from an AVR, 8-bit, bare-metal, single-developer C environment to a collaborative Unix world. I would now change a number of things, and will do so eventually. 

### Choice of preemptive and cooperative multithreading

The typical trick to preempt a thread and go back to the scheduler is using a longjmp from a signal handler. But there are problems with this:
* longjmp is not an an async-safe function, so is not supposed to be called from a signal handler. Well, OK, looks like in 2012 longjmp was [accepted as async-safe-with-limitations](http://austingroupbugs.net/view.php?id=516). Look at that discussion and tell me how safe you feel. More safety feelings [here](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1318.htm): did you know that there can be "undocumented quiet changes" between C standard revisions?
* in fact, CERT classifies longjmp-from-signal-handler as a [vulnerability](https://www.securecoding.cert.org/confluence/display/c/SIG30-C.+Call+only+asynchronous-safe+functions+within+signal+handlers)
* calling longjmp from the signal handler causes the kernel to consider the rest of the program execution to be still inside of a signal handler, which can have an effect on the delivery of other signals

So, additionally to the preemptive-multithreading mode, I have implemented a cooperative-multithreading mode, which is as well-behaved as possible concerning the POSIX / ISO C rules. The config.h file allows to select preemptive or collaborative thread scheduling:
* In the preemptive case, the context switches happen in a signal handler, so the thread doesn't have to do anything special. 
* In the collaborative case, the signal handler only notifies the expiry of the thread's quantum, and the thread is expected to call periodically mythread_yield(bool forced) to pass control to the scheduler. This function actually only yields when the quantum has expired or when forced=true.

Maybe interesting is that the preemptive scheduling sometimes segfaults in Linux (about 30% of the time?), but never seems to segfault in Mac OS X. 

### And still hopelessly hackish

Cooperative or preemptive, well-behaved or who-cares, it's important to remember that this kind of thing is doomed to be a hack. 
On one hand, it's been a long-lived and respawned hack, as can be seen on the paper ["Portable Multithreading - The Signal Stack Trick For User-Space Thread Creation"](http://www.gnu.org/software/pth/rse-pmt.ps) from the year 2000, which already reported and elaborated on 19 (!) userspace threading libraries, including the GNU Portable Threads library. 
On the other hand, on 2005 there's ["Threads cannot be implemented as a library"](http://lambda-the-ultimate.org/node/950), which boils down to "a language needs a concurrency-aware memory model to be able to guarantee correctness over concurrency". The author, Hans Boehm, later went on to chair the C++11 and C11 standards commitees to work on their memory models - and now C11 does have the concept of  threads. Yay. 

### Triggering signals

The more typical way to trigger a signal seems to be timer_create/alarm, but I was interested in making this work both on Linux and Mac OS X, so I used setitimer instead. It seems to be the best supported / less obsoleted option present in both OSes to get shorter-than-second alarm timings.

### Mutexes

Recursive mutexes have been implemented. It could be parametrized to make them non-recursive, or instrumented to catch inconsistency errors.

## Testing in Linux
On Ubuntu 13.4 I compiled using

gcc -std=gnu99 -Wall -O3 -U_FORTIFY_SOURCE main.c uthreads.c 

The -U switch is needed because fortify, which is default in Ubuntu for some time, [causes thread switching via longjmp to fail](http://permalink.gmane.org/gmane.comp.systems.archos.rockbox.cvs/32841)
