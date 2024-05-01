#include "./headers/ssi.h"

void klog_print();
void klog_print_dec();
void breakPoint();
extern struct list_head *readyQueue;
extern struct list_head *pseudoClockQueue;
extern pcb_PTR devQueue[DEVINTNUM][DEVPERINT];
extern int process_count;
extern int softBlockCount;

void initSSI(){
	systemServiceInterface();
}

void systemServiceInterface(){
	ssi_payload_PTR payload = NULL;
	pcb_PTR sender;
	
	while(TRUE){
		sender = (pcb_PTR)SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int)(&payload), 0);
		SSIRequest(sender, payload->service_code, payload->arg);
	}
}

void SSIRequest(pcb_t* sender, int service, void* arg){
	klog_print("SSI request ");
	klog_print_dec(service);
	klog_print("\n");
	switch(service){
		case CREATEPROCESS:
			createProcess(arg, sender);
			break;
		case TERMPROCESS:
			killProcess(arg, sender);
			break;
		case DOIO:
			doIO(arg, sender);
			break;
		case GETTIME:
			getTime(sender);
			break;
		case CLOCKWAIT:
			waitForClock(sender);
			break;
		case GETSUPPORTPTR:
			getSupportStruct(sender);
			break;
		case GETPROCESSID:
			getPID(sender);
			break;
		default:	
			klog_print("invalid SSI service\n");
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
	process_count--;
	freePcb(doomed);
}

void doIO(ssi_do_io_PTR arg, pcb_PTR sender){
	// calculating interrupt line and device number from commandAddr	
	int device = ((unsigned int)arg->commandAddr - DEVADDR) / DEVREGSIZE;	
	int intLineNo = device / DEVPERINT;
	int devNo = device % DEVPERINT;
	softBlockCount++;

	klog_print("interrupt line = ");
	klog_print_dec(intLineNo);
	klog_print("\ndevice number = ");
	klog_print_dec(devNo);
	klog_print("\n");
	
	devQueue[intLineNo][devNo] = outAnyProcQ(sender);
	
	klog_print("blocked pcb ");
	klog_print_dec(sender->p_pid);
	klog_print("\n");

	*arg->commandAddr = arg->commandValue;	
}

void getTime(pcb_PTR sender){
	SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned int)sender->p_time, 0);
}

void waitForClock(pcb_PTR sender){
	insertProcQ(pseudoClockQueue, outAnyProcQ(sender));
	SYSCALL(SENDMESSAGE, (unsigned int)sender, 0, 0);
}

void getSupportStruct(pcb_PTR sender){
	SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned int)&sender->p_supportStruct, 0);
}

void getPID(pcb_PTR sender){
	SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned int)sender->p_pid, 0);
}
