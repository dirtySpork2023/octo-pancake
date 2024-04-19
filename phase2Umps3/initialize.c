#include "../headers/const.h"
#include "../headers/types.h"
#include "../phase1/headers/pcb.h"
#include "headers/scheduler.h"
#include "headers/exceptions.h"
#include "headers/ssi.h"

extern void test();

/*	counter of all started but not yet terminated processes
	includes processes in "running", "ready" AND "blocked" state */
int process_count;
/*	counter of processes in the "blocked" state due to an I/O or timer request.
	doesn't include processes waiting for a message*/
int softBlockCount;
/*	pointer to process in running state, NULL when kernel is in WAIT() */
pcb_PTR current_process;
/*	pointer to SSI process */
pcb_PTR ssi_pcb;

/* READY PCBS */
struct list_head readyQueue; // tail pointer

/* BLOCKED PCBS */
/*	processes that requested WaitForClock service to the SSI
	resumed by: system-wide interval timer's interrupt */
struct list_head pseudoClockQueue;
/*	processes waiting for a message to be received
	resumed by: new message directed to them */
//struct list_head receiveMessageQueue;
// TODO I/O queues ?

// Lists of blocked PCBS for each device (o array di length SEMDEVLEN - 1) ?
struct list_head diskQueue;
struct list_head flashQueue;
struct list_head networkQueue;
struct list_head printerQueue;
// terminal devices are actually two independent sub-devices
struct list_head terminal1Queue; 
struct list_head terminal2Queue;

int main(){
	passupvector_t *passupvector = (passupvector_t *)PASSUPVECTOR;
	passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
	passupvector->tlb_refill_stackPtr = KERNELSTACK;
	passupvector->exception_handler = (memaddr)exceptionHandler;
	passupvector->exception_stackPtr = KERNELSTACK;

	initPcbs();
	initMsgs();
	process_count = 0;
	softBlockCount = 0;
	current_process = NULL;
	mkEmptyProcQ(&readyQueue);
	mkEmptyProcQ(&pseudoClockQueue);

	/* load System-wide Interval Timer with 100 milliseconds for pseudo-clock*/
	LDIT(PSECOND);

	// first process
	ssi_pcb = allocPcb();
	insertProcQ(&readyQueue, ssi_pcb);
	process_count++;
	ssi_pcb->p_s.status = ALLOFF | IEPON | IMON;
	ssi_pcb->p_s.pc_epc = (memaddr) initSSI;
	ssi_pcb->p_s.reg_t9 = (memaddr) initSSI;
	RAMTOP(ssi_pcb->p_s.reg_sp); // stack pointer = RAMTOP

	// second process
	pcb_PTR root = allocPcb();
	insertProcQ(&readyQueue, root);
	process_count++;
	root->p_s.status = ALLOFF | IEPON | IMON | TEBITON;
	root->p_s.pc_epc = (memaddr) test;
	root->p_s.reg_t9 = (memaddr) test;
	RAMTOP(root->p_s.reg_sp); // stack pointer = RAMTOP - 2*PAGESIZE
	root->p_s.reg_sp -= 2*PAGESIZE;

	scheduler();
	return 0;
}
