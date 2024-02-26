//#include <umps/libumps.h>
#include "headers/const.h"
#include "phase1/pcb.c"
#include "phase2Umps3/p2test.c"

// to be replaced in phase 3
void uTLB_RefillHandler() {
	setENTRYHI(0x80000000);
	setENTRYLO(0x00000000);
	TLBWR();
	LDST((state_t*) 0x0FFFF000);
}

void exceptionHandler() {
	// TODO
}

int main(){
	// GLOBAL VARS

	// started, but not yet terminated processes
	int processCount;
	// started processes, in the “blocked” state due to an I/O or timer request.
	int softBlockCount;
	pcb_PTR currentProcess;
	// tail pointer to queue of PCBs in ready state
	struct list_head *readyQueue;
	// TODO blocked PCBs
	
	passupvector_t *passupvector = PASSUPVECTOR;
	passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
	passupvector->tlb_refill_stackPtr = KERNELSTACK;
	passupvector->exception_handler = (memaddr)exceptionHandler; // TODO function defined above
	passupvector->exception_stackPtr = KERNELSTACK;

	initPcbs();
	initMsgs();
	processCount = 0;
	softBlockCount = 0;
	currentProcess = NULL;
	mkEmptyProcQ(readyQueue);
	
	LDIT(PSECOND);

	pcb_PTR pcb1 = allocPcb();
	insertProcQ(readyQueue, pcb1);
	processCount++;
	pcb1->p_s.status = STATUSINIT;
	pcb1->p_s.pc_epc = (memaddr) test; // function in p2test.c or defined as extern ?
	pcb1->p_s.reg_t9 = (memaddr) test;
	// TODO stack pointer = RAMTOP
	// ? LDST()
	// ? LDCXT (,pcb1->p_s.status, pcb1->p_s.pc_epc);

	pcb_PTR pcb2 = allocPcb();
	insertProcQ(readyQueue, pcb2);
	processCount++;
	pcb1->p_s.status = STATUSINIT;
	pcb1->p_s.pc_epc = (memaddr) test; // function in p2test.c or defined as extern ?
	pcb1->p_s.reg_t9 = (memaddr) test;
	// TODO stack pointer = RAMTOP - (2 * FRAMESIZE)

	// scheduler();
}