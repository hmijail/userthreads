

#ifndef UTHREADS_H
#define UTHREADS_H

// Refer to the readme to learn about differences, motivation and usage of both types of scheduling
#ifndef CONFIG_PREEMPTIVE_SCHEDULING
//#define CONFIG_PREEMPTIVE_SCHEDULING   true
#define CONFIG_PREEMPTIVE_SCHEDULING   false
#endif

//This logs the most important checkpoints of the program
#ifndef CONFIG_VERBOSE
#define CONFIG_VERBOSE                 false
//#define CONFIG_VERBOSE                 true
#endif

#endif // UTHREADS_H
