#include "./headers/scheduler.h"

/* to be replaced in phase 3 */
void uTLB_RefillHandler(){
	setENTRYHI(0x80000000);
	setENTRYLO(0x00000000);
	TLBWR();
	LDST((state_t*) 0x0FFFF000);
}

void interruptHandler(int cause){
	/* interrupt line 0 ignored */
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
	if(cause & DISKINTERRUPT){
		
	}
	if(cause & FLASHINTERRUPT){
		
	}
	/* interrupt line 5 ignored */
	if(cause & PRINTINTERRUPT){
		
	}
	if(cause & TERMINTERRUPT){

	}
}

/* handles all exceptions, not TLB-Refill events */
void exceptionHandler(void){
	/* the processor state at the time of the exception will
have been stored at the start of the BIOS Data Page */
	/* processor already set to kernel mode and disabled interrupts*/
	unsigned int cause = getCAUSE();
	unsigned int excCode = (cause & GETEXECCODE) >> CAUSESHIFT;
	
	/* "break" command will not be executed if kernel works properly */
	switch(excCode){
		case IOINTERRUPTS:
			interruptHandler(cause);
			break;
		case TLBINVLDL:

			break;
		case TLBINVLDS:

			break;
		case SYSEXCEPTION:

			break;
		case BREAKEXCEPTION:

			break;
		case PRIVINSTR:

			break;
		default:
			break;
	}

	
}

void scheduler(){
	if(emptyProcQ(readyQueue)){
		/*	HALT execution: if there are no more processes to run
			WAIT for an I/O operation to complete: which will unblock a PCB and populate the Ready Queue
			PANIC: halt execution in the presence of deadlock */
	
		if(processCount == 1) /* TODO and the SSI is the only process in the system */
			HALT(); /* HALT BIOS service/instruction */
		if(processCount > 0 && softBlockCount > 0){
			currentProcess = NULL;
			unsigned int waitStatus = getSTATUS();
			/* enable all interrupts and disable PLT */
			waitStatus &= !IMON;
			waitStatus &= !IEPON;
			waitStatus &= !TEBITON;
			setSTATUS(waitStatus);
			WAIT(); /* enter a Wait State */
		}
		if(processCount > 0 && softBlockCount == 0)
			PANIC(); /* PANIC BIOS service/instruction*/
	}

	currentProcess = removeProcQ(readyQueue);
	/* load round-robin timeslice into Processor's Local Timer */
	setTIMER(TIMESLICE);
	/* load the processor state of the current process */
	LDST(&currentProcess->p_s);
}