#include "./headers/scheduler.h"

extern int process_count;
extern int softBlockCount;
extern pcb_PTR current_process;
extern struct list_head readyQueue;

/* assuming old process was saved
sets currentProcess to another PCB*/
void scheduler(){
	/*for debugging*/
	if(current_process != NULL){
		klog_print("process not saved correctly\n");
		breakPoint();
	}

	if(emptyProcQ(&readyQueue)){
		if(process_count == 1)
			/* the SSI is the only process in the system */
			HALT(); /* HALT BIOS service/instruction */
		if(process_count > 0 && softBlockCount == 0)
			/* deadlock */
			PANIC(); /* PANIC BIOS service/instruction*/
		else if(process_count > 1 && softBlockCount > 0){
			/* all pcbs are waiting for an I/O operation to complete */
			current_process = NULL;
			unsigned int waitStatus = getSTATUS();
			/* enable all interrupts and disable PLT */
			waitStatus &= !IMON;
			waitStatus &= !IEPON;
			waitStatus &= !TEBITON;
			setSTATUS(waitStatus);
			WAIT(); /* enter a Wait State */
		}
	}

	klog_print("scheduling\n");
	current_process = removeProcQ(&readyQueue);
	/* load round-robin timeslice into Processor's Local Timer */
	setTIMER(TIMESLICE);
	/* load the processor state of the current process */
	LDST(&current_process->p_s);
}