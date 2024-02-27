#ifndef KERNEL_H_INCLUDED
#define KERNEL_H_INCLUDED

#include "../../headers/const.h"
#include "../../headers/types.h"
#include "phase1/pcb.c"
#include "p2test.c"
#include "scheduler.c"

/* started, but not yet terminated processes */
int processCount;
/* started processes, in the “blocked” state due to an I/O or timer request. */
int softBlockCount;
pcb_PTR currentProcess;
struct list_head *readyQueue; // tail pointer
// TODO blocked PCBs queue

#endif