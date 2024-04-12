#include "./headers/ssi.h"

extern struct list_head *pseudoClockQueue;

void initSSI(){
	// TODO make SSI available for other processes to send messages
	systemServiceInterface();
}

void systemServiceInterface(){
	ssi_payload_t payload;
	pcb_PTR sender;
	
	while(TRUE){
		sender = SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int)&payload, 0);
		switch(payload.service_code){
			case CREATEPROCESS:
				createProcess(payload.arg, sender);
			case TERMPROCESS:
				killProcess(payload.arg, sender);
			case DOIO:
				doIO(payload.arg);
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
				breakPoint();
		}
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
		SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned int)newChild, 0);
	}
}

void killProcess(pcb_PTR doomed, pcb_PTR sender){
	if(doomed == NULL) doomed = sender;
	
	while(!emptyChild(doomed)){
		killProcess(removeChild(doomed), NULL);
	}
	// TODO 
	// Manca qualcoa secondo me

	outChild(doomed);
	freePcb(doomed);
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