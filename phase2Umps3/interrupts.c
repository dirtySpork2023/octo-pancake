#include "./headers/interrupts.h"

void interruptHandler(int cause){

	/* timer interrupts */

	if(cause & LOCALTIMERINT){
		/*	timer will be reset in scheduler() */
		/*	save processor state to ready queue */
		state_t *dataPage = BIOSDATAPAGE;
		currentProcess->p_s.entry_hi = dataPage->entry_hi;
		currentProcess->p_s.cause = dataPage->cause;
		currentProcess->p_s.status = dataPage->status;
		currentProcess->p_s.pc_epc = dataPage->pc_epc;
		for(int i=0; i<29; i++)
			currentProcess->p_s.gpr[i] = dataPage->gpr[i];
		currentProcess->p_s.hi = dataPage->hi;
		currentProcess->p_s.lo = dataPage->lo;
		insertProcQ(readyQueue, currentProcess);
		scheduler();
	}

	if(cause & TIMERINTERRUPT){
		/* re-set Interval Timer with 100 milliseconds */
		LDIT(PSECOND);
		/*	unlock all pcbs in pseudoClockQueue */
		while(!emptyProcQ(pseudoClockQueue)){
			softBlockCount--;
			insertProcQ(readyQueue, removeProcQ(pseudoClockQueue));
		}
		if(currentProcess != NULL){
			// only if there was a process running before the interrupt
			// load processor state stored at address BIOSDATAPAGE
			LDST(BIOSDATAPAGE);
		}else{
			scheduler();
		}
	}

	/* device interrupts */

	if(cause & DISKINTERRUPT){
		
	}
	if(cause & FLASHINTERRUPT){
		
	}
	if(cause & PRINTINTERRUPT){
		
	}
	if(cause & TERMINTERRUPT){

	}

	/* interrupt lines 0 and 5 are ignored */
}