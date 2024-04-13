#include "./headers/ssi.h"

extern struct list_head *readyQueue;
extern struct list_head *pseudoClockQueue;
extern int process_count;
extern int softBlockCount;

void initSSI(){
	
	systemServiceInterface();
}

void systemServiceInterface(){
	ssi_payload_PTR payload;
	pcb_PTR sender;
	
	while(TRUE){
		sender = (pcb_PTR)SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int)payload, 0);
		SSIRequest(sender, payload->service_code, payload->arg);
		// SYSCALL(SENDMESSAGE, risposta? )
	}
}

void SSIRequest(pcb_t* sender, int service, void* arg){
	switch(service){
		case CREATEPROCESS:
			createProcess(arg, sender);
		case TERMPROCESS:
			killProcess(arg, sender);
		case DOIO:
			doIO(arg);
		case GETTIME:
			getTime(sender);
		case CLOCKWAIT:
			waitForClock(sender);
		case GETSUPPORTPTR:
			getSupportStruct(sender);
		case GETPROCESSID:
			getPID(sender);
		default:
			klog_print("SSI call invalid\n");
			killProcess(sender, sender);
			breakPoint();
	}
}

void createProcess(ssi_create_process_PTR arg, pcb_PTR sender){
	pcb_PTR newChild = allocPcb();
	if(newChild == NULL){
		/* no free PCBs available */
		sender->p_s.reg_v0 = NOPROC;
	}else{
		copyState(arg->state, &newChild->p_s);
		newChild->p_supportStruct = arg->support;
		insertChild(sender, newChild);
		insertProcQ(readyQueue, newChild);
		process_count++;
		SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned int)newChild, 0);
	}
}

void killProcess(pcb_PTR doomed, pcb_PTR sender){
	if(doomed == NULL) doomed = sender;
	
	while(!emptyChild(doomed)){
		killProcess(removeChild(doomed), NULL);
	}

	//remove from sibling list
	outChild(doomed);
	//remove from any process queue
	if(outProcQ(readyQueue, doomed) == NULL){
		softBlockCount--;
		outAnyProcQ(doomed);
	}
	freePcb(doomed);
	process_count--;
}

void doIO(ssi_do_io_PTR arg){
	//TODO
}

void getTime(pcb_PTR sender){
	SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned int)sender->p_time, 0);
}

void waitForClock(pcb_PTR sender){
	insertProcQ(pseudoClockQueue, sender);
}

void getSupportStruct(pcb_PTR sender){
	SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned int)&sender->p_supportStruct, 0);
}

void getPID(pcb_PTR sender){
	SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned int)sender->p_pid, 0);
}
