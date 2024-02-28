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
	/* the processor state at the time of the exception will
have been stored at the start of the BIOS Data Page */
	// TODO
}

void scheduler(){
	if(emptyProcQ(readyQueue)){
		/*	HALT execution: if there are no more processes to run
			WAIT for an I/O operation to complete: which will unblock a PCB and populate the Ready Queue
			PANIC: halt execution in the presence of deadlock
			
			If the Process Count is 1 and the SSI is the only process in
the system, invoke the HALT BIOS service/instruction
			If the Process Count > 0 and the Soft-block Count > 0 enter
a Wait State
			If the Process Count > 0 and the Soft-block Count is 0 invoke
the PANIC BIOS service/instruction*/
	}

	currentProcess = removeProcQ(readyQueue);
	/* load round-robin timeslice into Processor's Local Timer */
	setTIMER(TIMESLICE);
	/* load the processor state of the current process */
	LDST(&currentProcess->p_s);
}