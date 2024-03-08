#include "./headers/ssi.h"

void initSSI(){
	systemServiceInterface();
}

void systemServiceInterface(){
	ssi_payload_t payload;
	pcb_PTR sender;
	while(TRUE){
		sender = SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, &payload, 0);
		switch(payload.service_code){
			case CREATEPROCESS:
				createProcess(payload.arg, sender);
			case TERMPROCESS:
				killProcess(payload.arg);
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
		}
	}
}

void createProcess(ssi_create_process_PTR arg, pcb_PTR sender){
	pcb_PTR newChild = allocPcb();
	if(newChild == NULL){
		/* no free PCBs available */
		currentProcess->p_s.reg_v0 = NOPROC;
	}else{
		copyState(arg->state, &newChild->p_s);
		newChild->p_supportStruct = &arg->support;
		insertChild(currentProcess, newChild);
		SYSCALL(SENDMESSAGE, sender, newChild, 0);
	}
}

void killProcess(pcb_PTR dead){
	if(dead == NULL) dead = currentProcess;
	
	while(!emptyChild(dead)){
		killProcess(removeChild(dead));
	}
	
	outChild(dead);
	freePcb(dead);
}

void doIO(ssi_do_io_PTR arg){
	//TODO
}

void getTime(pcb_PTR sender){
	SYSCALL(SENDMESSAGE, sender, sender->p_time, 0);
}

void waitForClock(pcb_PTR sender){
	insertProcQ(pseudoClockQueue, sender);
}

void getSupportStruct(pcb_PTR sender){
	SYSCALL(SENDMESSAGE, sender, &sender->p_supportStruct, 0);
}

void getPID(pcb_PTR sender){
	SYSCALL(SENDMESSAGE, sender, sender->p_pid, 0);
}