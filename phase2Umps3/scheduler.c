#include "./headers/scheduler.h"

/* to be replaced in phase 3 */
void uTLB_RefillHandler(){
	setENTRYHI(0x80000000);
	setENTRYLO(0x00000000);
	TLBWR();
	LDST((state_t*) 0x0FFFF000);
}

void interruptHandler(int cause){
	if(cause & LOCALTIMERINT){
		/*	Copy the processor state at the time of the exception (located at the start of the BIOS Data
			Page [Section 3.2.2-pops]) into the Current Process’s PCB (p_s).12*/
		currentProcess->p_s.cause = getCAUSE();
		currentProcess->p_s.entry_hi = getENTRYHI();
		currentProcess->p_s.pc_epc = getEPC();
		currentProcess->p_s.status = getSTATUS();
		//TODO save other registers
		insertProcQ(readyQueue, currentProcess);
		scheduler();
	}
	if(cause & TIMERINTERRUPT){
		
	}
	if(cause & DISKINTERRUPT){
		
	}
	if(cause & FLASHINTERRUPT){
		
	}
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
	unsigned int excCode = (cause & GETEXECCODE)/4;
	
	switch((cause & GETEXECCODE)/4){
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
			unsigned int tmpStatus = getSTATUS();
			/* enable all interrupts and disable PLT */
			tmpStatus &= !IMON;
			tmpStatus &= !IEPON;
			tmpStatus &= !TEBITON;
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