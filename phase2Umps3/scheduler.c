#include "./headers/scheduler.h"

void klog_print();
void klog_print_dec();
void klog_print_hex();
void breakPoint();
extern int process_count;
extern int softBlockCount;
extern pcb_PTR current_process;
extern struct list_head readyQueue;

/* assuming old process was saved
sets currentProcess to another PCB*/
void scheduler(){
	/*for debugging*/
	if(current_process != NULL){
		klog_print("ERR: process not saved correctly\n");
		breakPoint();
	}

	if(emptyProcQ(&readyQueue)){
		if(process_count == 1){
			/* the SSI is the only process in the system */
			klog_print("HALT STATE\n");
			HALT(); /* HALT BIOS service/instruction */
		}else if(process_count > 1 && softBlockCount > 0){
			/* all pcbs are waiting for an I/O operation to complete */
			unsigned int waitStatus = getSTATUS();
			/* enable all interrupts and disable PLT */
			waitStatus |= IMON | IEPON;
			waitStatus &= !TEBITON;
			setSTATUS(waitStatus);
			klog_print("WAIT STATE ");
			WAIT(); /* enter a Wait State */
			klog_print_hex(getSTATUS());
			klog_print("HAPPENED\n");
			// TODO why the fuck is the wait state not waiting and why the fuck is the device not interrupting god fucking fuck
			scheduler();
		}else if(process_count > 0 && softBlockCount == 0){
			/* deadlock */
			klog_print("DEADLOCK PANIC STATE\n");
			PANIC(); /* PANIC BIOS service/instruction*/
		}else{
			klog_print("ERR: empty ready queue\n");
			breakPoint();
		}
	}

	current_process = removeProcQ(&readyQueue);
	
	klog_print("pcb");
	klog_print_dec(current_process->p_pid);
	klog_print(": ");
	
	/* load round-robin timeslice into Processor's Local Timer */
	setTIMER(TIMESLICE);
	
	/* load the processor state of the current process */
	LDST(&current_process->p_s);
}
