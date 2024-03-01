//#include <libumps> 
#include "headers/const.h"
#include "phase1/pcb.c"

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
	// TODO tail pointer to queue of PCBs in ready state
	// TODO blocked PCBs

	passupvector_t *passupvector = PASSUPVECTOR;
	passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
	passupvector->tlb_refill_stackPtr = KERNELSTACK;
	passupvector->exception_handler = (memaddr)exceptionHandler; // TODO function defined above
	passupvector->exception_stackPtr = KERNELSTACK;

	initPcbs();
	initMsgs();

	// TODO
	// processCount = 
	// softBlockCount =
	// currentProcess =
	// init queues

	while(1){
		// scheduler ?
	}

}