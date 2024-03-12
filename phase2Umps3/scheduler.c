#include "./headers/scheduler.h"

extern int process_count;
extern int softBlockCount;
extern pcb_PTR current_process;
extern struct list_head *readyQueue;

/* assuming old process was saved
sets currentProcess to another PCB*/
void scheduler(){
	if(emptyProcQ(readyQueue)){
		/*	HALT execution: if there are no more processes to run
			WAIT for an I/O operation to complete: which will unblock a PCB and populate the Ready Queue
			PANIC: halt execution in the presence of deadlock */
	
		if(process_count == 1) /* TODO and the SSI is the only process in the system */
			HALT(); /* HALT BIOS service/instruction */
		if(process_count > 0 && softBlockCount > 0){
			current_process = NULL;
			unsigned int waitStatus = getSTATUS();
			/* enable all interrupts and disable PLT */
			waitStatus &= !IMON;
			waitStatus &= !IEPON;
			waitStatus &= !TEBITON;
			setSTATUS(waitStatus);
			WAIT(); /* enter a Wait State */
		}
		if(process_count > 0 && softBlockCount == 0)
			PANIC(); /* PANIC BIOS service/instruction*/
	}

	current_process = removeProcQ(readyQueue);
	/* load round-robin timeslice into Processor's Local Timer */
	setTIMER(TIMESLICE);
	/* load the processor state of the current process */
	LDST(&current_process->p_s);
}