#include "../headers/const.h"
#include "../headers/types.h"
#include "../phase1/headers/pcb.h"
#include "headers/scheduler.h"
#include "headers/exceptions.h"
#include "headers/interrupts.h"
#include "headers/ssi.h"

#define FRAMESIZE 1024
extern void test();

/* counter of all started but not yet terminated processes
	includes processes in "running", "ready" AND "blocked" state */
int processCount;
/* counter of processes in the "blocked" state due to an I/O or timer request. */
int softBlockCount;
/* pointer to process in running state, NULL when kernel is in WAIT() */
pcb_PTR currentProcess;
/* pointer to SSI process */
pcb_PTR SSI;

/* READY PCBS */
struct list_head *readyQueue; // tail pointer

/* BLOCKED PCBS */
/*	processes that requested WaitForClock service to the SSI
	resumed by: system-wide interval timer's interrupt */
struct list_head *pseudoClockQueue;
/*	processes waiting for a message to be received
	resumed by: new message directed to them */
struct list_head *receiveMessageQueue;
// TODO I/O queues ?

int main(){
	passupvector_t *passupvector = (passupvector_t *)PASSUPVECTOR;
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
	mkEmptyProcQ(receiveMessageQueue);
	/* load System-wide Interval Timer with 100 milliseconds */
	LDIT(PSECOND);

	// first test process
	SSI = allocPcb();
	insertProcQ(readyQueue, SSI);
	processCount++;
	SSI->p_s.status &= !IMON;
	SSI->p_s.status &= !IEPON; // interrupt enabled (== interrupt mask disabled)
	SSI->p_s.status &= !USERPON; // user mode disabled
	SSI->p_s.status |= TEBITON; // local timer on
	SSI->p_s.pc_epc = (memaddr) initSSI;
	SSI->p_s.reg_t9 = (memaddr) initSSI;
	RAMTOP(SSI->p_s.reg_sp); // stack pointer = RAMTOP

	// second test process
	pcb_PTR root = allocPcb();
	insertProcQ(readyQueue, root);
	processCount++;
	root->p_s.status &= !IMON;
	root->p_s.status &= !IEPON; // interrupt enabled (== interrupt mask disabled)
	root->p_s.status &= !USERPON; // user mode disabled
	root->p_s.status |= TEBITON; // local timer on
	root->p_s.pc_epc = (memaddr) test;
	root->p_s.reg_t9 = (memaddr) test;
	RAMTOP(root->p_s.reg_sp); // stack pointer = RAMTOP
	root->p_s.reg_sp -= 2*FRAMESIZE; // TODO ???

	scheduler();
	return 0;
}