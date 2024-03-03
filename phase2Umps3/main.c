#include "../headers/const.h"
#include "../headers/types.h"
#include "../phase1/pcb.c"
#include "headers/scheduler.h"
#include "headers/exceptions.h"
#include "p2test.c"

/*	counter of all started but not yet terminated processes
	includes processes in "running", "ready" AND "blocked" state */
int processCount;
/*	counter of processes in the "blocked" state due to an I/O or timer request. */
int softBlockCount;
/*	pointer to process in running state, NULL when kernel is in WAIT() */
pcb_PTR currentProcess;

/* READY PCBS */
struct list_head *readyQueue; // tail pointer

/* BLOCKED PCBS */
/*	processes enter this queue when they request a WaitForClock service to the SSI
	they get resumed with the system-wide interval timer's interrupt */
struct list_head *pseudoClockQueue;

// TODO other blocked PCBs queues ?

int main(){
	passupvector_t *passupvector = PASSUPVECTOR;
	passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
	passupvector->tlb_refill_stackPtr = KERNELSTACK;
	passupvector->exception_handler = (memaddr)exceptionHandler;
	passupvector->exception_stackPtr = KERNELSTACK;

	initPcbs();
	initMsgs();
	processCount = 0;
	softBlockCount = 0;
	currentProcess = NULL;
	mkEmptyProcQ(readyQueue);
	mkEmptyProcQ(pseudoClockQueue);
	/* load System-wide Interval Timer with 100 milliseconds */
	LDIT(PSECOND);

	// first test process
	pcb_PTR pcb1 = allocPcb();
	insertProcQ(readyQueue, pcb1);
	processCount++;
	pcb1->p_s.status &= !IMON;
	pcb1->p_s.status &= !IEPON; // interrupt enabled (== interrupt mask disabled)
	pcb1->p_s.status &= !USERPON; // user mode disabled
	pcb1->p_s.status |= TEBITON; // local timer on
	pcb1->p_s.pc_epc = (memaddr) test;
	pcb1->p_s.reg_t9 = (memaddr) test;
	RAMTOP(pcb1->p_s.reg_sp); // stack pointer = RAMTOP

	// second test process
	pcb_PTR pcb2 = allocPcb();
	insertProcQ(readyQueue, pcb2);
	processCount++;
	pcb2->p_s.status &= !IMON;
	pcb2->p_s.status &= !IEPON; // interrupt enabled (== interrupt mask disabled)
	pcb2->p_s.status &= !USERPON; // user mode disabled
	pcb2->p_s.status |= TEBITON; // local timer on
	pcb2->p_s.pc_epc = (memaddr) test;
	pcb2->p_s.reg_t9 = (memaddr) test;
	RAMTOP(pcb2->p_s.reg_sp); // stack pointer = RAMTOP
	pcb2->p_s.reg_sp -= 2*FRAMESIZE; // TODO quanto spazio alloca il processo test ???

	scheduler();
}