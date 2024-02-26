#include "./headers/interrupts.h"

void interruptHandler(int cause){

	/* timer interrupts */

	if(cause & LOCALTIMERINT){
		/*	timer will be reset in scheduler() */
		copyState(BIOSDATAPAGE, &current_process->p_s);
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