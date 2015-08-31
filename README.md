# C/POSIX portable userland threads

This is a microproject/proof-of-concept that I made some time ago. It needs some cleaning up and polishing, but it's already working, so good enough for a first public commit.

I have seen other implementations, and it's interesting how different can they be. So in the following I document my design choices. 

But first of all, the non-choice: this was made when I was transitioning from an AVR, 8-bit, bare-metal, single-developer C environment to a collaborative Unix world. I would now change a number of things, and will do so eventually. 

On the context of design choices and tricks or hacks, the paper ["Portable Multithreading - The Signal Stack Trick For User-Space Thread Creation"](http://www.gnu.org/software/pth/rse-pmt.ps) from the year 2000 is pretty interesting. It already reported and elaborated on 19 (!) userspace threading libraries, including the GNU Portable Threads library. 

### Choice of preemptive and cooperative multithreading

The typical trick to preempt a thread and go back to the scheduler is using a longjmp from a signal handler. But there are problems with this:
* longjmp is not an an async-safe function, so is not supposed to be called from a signal handler. Well, OK, looks like in 2012 longjmp was [accepted as async-safe-with-limitations](http://austingroupbugs.net/view.php?id=516). Look at that discussion and tell me how safe you feel. More safety feelings [come from the C standard itself](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1318.htm): did you know that there can be "undocumented quiet changes" between C standard revisions? Safey safey!
* in fact, CERT classifies longjmp-from-signal-handler as a [vulnerability](https://www.securecoding.cert.org/confluence/display/c/SIG30-C.+Call+only+asynchronous-safe+functions+within+signal+handlers)
* calling longjmp from the signal handler causes the kernel to consider the rest of the program execution to be still inside of a signal handler, which can have an effect on the delivery of other signals

So, additionally to the preemptive-multithreading mode, I have implemented a cooperative-multithreading mode, which is as well-behaved as possible concerning the POSIX / ISO C rules. The config.h file allows to select preemptive or collaborative thread scheduling:
* In the preemptive case, the context switches happen in a signal handler, so the thread doesn't have to do anything special. 
* In the collaborative case, the signal handler only notifies the expiry of the thread's quantum by setting a variable, and the thread is expected to call periodically mythread_yield(bool forced) to pass control to the scheduler. This function can be called in such a way that it yields unconditionally, or conditionally to the quantum having expired.

Maybe interesting is that the preemptive scheduling sometimes segfaults in Linux (about 30% of the time?), but never seems to segfault in Mac OS X. 

### And still hopelessly hackish

Cooperative or preemptive, well-behaved or who-cares, it's important to remember that this kind of thing is doomed to be an unreliable hack, because ["Threads cannot be implemented as a library"](http://lambda-the-ultimate.org/node/950). That is a paper from 2005, which boils down to "a language needs a concurrency-aware memory model to be able to guarantee correctness over concurrency". 

Of course that only ("only") means that you can't reliably implement threads from inside the language; when you use help from the outside (like assembler) then, well, maybe, who knows. But since the purpose of this microproject is to stay inside of C/POSIX, well... good luck. If anything, optimization is one of the things than can trigger non-threading-compatible behaviour, so better use the lowest optimization possible.

Anyway, the author of that threads vs. libs paper, Hans Boehm, procceeded then to chair the C++11 and C11 standard commitees on the subject of concurrency and memory models, and so they have one now. So maybe a pure-C11 implementation could be reliable? No idea; I did this on C99, using C11 might be an interesting comparison.

### Triggering signals

The more typical way to trigger a signal seems to be timer_create/alarm, but I was interested in making this work both on Linux and Mac OS X, so I used setitimer instead. It seems to be the best supported / less obsoleted option present in both OSes to get shorter-than-second alarm timeouts.

### Mutexes

Recursive mutexes have been implemented. They could be parametrized to make them non-recursive, or instrumented to catch inconsistency errors.

To exercise and test the mutexes, the classical [Dinning Philosophers problem](https://en.wikipedia.org/wiki/Dining_philosophers_problem) is implemented in main: each of the 4 dining philosophers (threads) will try to grab their couple of forks (mutexes) 100 times by default (can be changed by giving a new number as the first command line argument). Soon they will deadlock, and the scheduler will notice it and end the program, printing the number of dead-locked threads.

There are other example threads implemented, like a smarter_philosopher() which will return its fork (mutex) to the table if he notices that the 2nd fork is busy - thus avoiding deadlock, but still risking livelock/starvation.

### Design choices

The infrastructure needed to store the runnable threads and mutexes is init'ed statically inside the uthreads library. On some systems that could be a virtue, like on an embedded system where one should not use malloc and know exactly how many threads will be running. (AVR offers longjmp; signals would have to be changed for IRQs).

But on more dynamic systems, that infrastructure should probably use some dynamic list of threads and mutexes.

## Testing in Linux
On Ubuntu, compile with

gcc -std=gnu99 -Wall -O3 -U_FORTIFY_SOURCE main.c uthreads.c 

The -U switch is needed because fortify, which is default in Ubuntu for some time, [causes thread switching via longjmp to fail](http://permalink.gmane.org/gmane.comp.systems.archos.rockbox.cvs/32841)

