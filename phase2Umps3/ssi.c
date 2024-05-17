#include "./headers/ssi.h"

void klog_print();
void klog_print_dec();
void breakPoint();
extern struct list_head pcbFree_h;
extern pcb_PTR current_process;
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
			if(arg !=NULL)
				SYSCALL(SENDMESSAGE, (unsigned int)sender, 0, 0);
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
			getPID(arg, sender);
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

pcb_PTR outDevQ(pcb_PTR doomed){
	for(int i=0; i<DEVINTNUM; i++){
		for(int j=0; j<DEVPERINT; j++){
			if(devQueue[i][j] == doomed){
				devQueue[i][j] = NULL;
				return doomed;
			}
		}
	}
	return NULL;
}

void killProcess(pcb_PTR doomed, pcb_PTR sender){
	if(doomed == NULL) doomed = sender;

/*	struct list_head p_list;
    struct pcb_t *p_parent;
    struct list_head p_child;
    struct list_head p_sib;

	p_child list is the sentinel element of p_sib list
*/

	if(searchProcQ(&pcbFree_h, doomed) == doomed){
		klog_print("doomed pcb already dead!\n");
		breakPoint();
		return;
	}

	// detach from parent
	// remove from sibling list
	outChild(doomed);

	// kill all children
	while(!emptyChild(doomed)){
		killProcess(removeChild(doomed), NULL);
	}

	//remove from any process queue
	if(current_process == doomed){
	}else if(searchProcQ(readyQueue, doomed) == doomed){
		outAnyProcQ(doomed);
	}else if(searchProcQ(pseudoClockQueue, doomed) == doomed){
		softBlockCount--;
		outAnyProcQ(doomed);
	}else{
		softBlockCount--;
		outDevQ(doomed);
	}
	
	klog_print("killing pcb ");
	klog_print_dec(doomed->p_pid);
	klog_print("\n");

	process_count--;
	freePcb(doomed);
}

void doIO(ssi_do_io_PTR arg, pcb_PTR sender){
	// calculating interrupt line and device number from commandAddr	
	int device = ((unsigned int)arg->commandAddr - DEVADDR) / DEVREGSIZE;	
	int intLineNo = device / DEVPERINT;
	int devNo = device % DEVPERINT;
	softBlockCount++;

	devQueue[intLineNo][devNo] = outAnyProcQ(sender);
	
/*	klog_print("blocked pcb ");
	klog_print_dec(sender->p_pid);
	klog_print(" for I/O\n");*/

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
	SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned int)sender->p_supportStruct, 0);
}

void getPID(void* arg, pcb_PTR sender){
/*	unsigned int reply;
	
	if( arg==0 ){
		reply = sender->p_pid;
	}else if( sender->p_parent==NULL ){
		reply = 0;
	}else{
		reply = sender->p_parent->p_pid;
	}

	SYSCALL(SENDMESSAGE, (unsigned int)sender, reply, 0);*/

	if( arg==0 )
		SYSCALL(SENDMESSAGE, (unsigned int)sender, sender->p_pid, 0);
	else if( sender->p_parent==NULL )
		SYSCALL(SENDMESSAGE, (unsigned int)sender, 0, 0);
	else
		SYSCALL(SENDMESSAGE, (unsigned int)sender, sender->p_parent->p_pid, 0);
}
