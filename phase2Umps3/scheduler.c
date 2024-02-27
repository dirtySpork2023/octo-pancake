#include "./headers/scheduler.h"

/* to be replaced in phase 3 */
void uTLB_RefillHandler(){
	setENTRYHI(0x80000000);
	setENTRYLO(0x00000000);
	TLBWR();
	LDST((state_t*) 0x0FFFF000);
}

/* handles all other exceptions, not TLB-Refill events */
void exceptionHandler(){
	// TODO
}

void scheduler(){
	currentProcess = removeProcQ(readyQueue);
	/* load round-robin timeslice into Processor's Local Timer */
	setTIMER(TIMESLICE);
	/* Perform a Load Processor State (LDST) on the processor state stored in PCB of the Current
Process (p_s). */
	LDST(&currentProcess->p_s);
}