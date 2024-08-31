#include "./headers/ssi.h"

void klog_print();
void klog_print_dec();
void breakPoint();
extern struct list_head pcbFree_h;
extern pcb_PTR current_process;
extern struct list_head readyQueue;
extern struct list_head receiveMessageQueue;
extern struct list_head pseudoClockQueue;
extern pcb_PTR devQueue[DEVINTNUM][DEVPERINT];
extern int process_count;
extern int softBlockCount;

static void createProcess(ssi_create_process_PTR arg, pcb_PTR sender){
	#ifdef DEBUG_SSI
	klog_print("SSI createProcess\n");
	#endif
	
	pcb_PTR newChild = allocPcb();
	if(newChild == NULL){
		/* no free PCBs available */
		sender->p_s.reg_v0 = NOPROC;
	}else{
		copyState(arg->state, &newChild->p_s);
		newChild->p_supportStruct = arg->support;
		insertChild(sender, newChild);
		insertProcQ(&readyQueue, newChild);
		process_count++;
		SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned int)newChild, 0);
	}
}

static void getSupportStruct(pcb_PTR sender){
	#ifdef DEBUG_SSI
	klog_print("SSI getSupportStruct\n");
	#endif
	
	SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned int)sender->p_supportStruct, 0);
}

static void killCall(void* arg, pcb_PTR sender){
	#ifdef DEBUG_SSI
	klog_print("SSI kill\n");
	#endif
	
	killProcess(arg, sender);
	if(arg != NULL)
		SYSCALL(SENDMESSAGE, (unsigned int)sender, 0, 0);
}

static void doIO(ssi_do_io_PTR arg, pcb_PTR sender){
	#ifdef DEBUG_SSI
	klog_print("SSI doIO\n");
	#endif

	// calculating interrupt line and device number from commandAddr	
	int device = ((unsigned int)arg->commandAddr - DEVADDR) / DEVREGSIZE;	
	int intLineNo = device / DEVPERINT;
	int devNo = device % DEVPERINT;
	softBlockCount++;

	devQueue[intLineNo][devNo] = outAnyProcQ(sender);

	#ifdef DEBUG_IO
	klog_print("SSI blocked pcb ");
	klog_print_dec(sender->p_pid);
	klog_print(" for I/O\n");
	#endif

	*arg->commandAddr = arg->commandValue;
}

static void getTime(pcb_PTR sender){
	#ifdef DEBUG_SSI
	klog_print("SSI getTime\n");
	#endif
	
	SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned int)sender->p_time, 0);
}

static void waitForClock(pcb_PTR sender){
	#ifdef DEBUG_SSI
	klog_print("SSI waitForClock\n");
	#endif
	
	insertProcQ(&pseudoClockQueue, outAnyProcQ(sender));
	softBlockCount++;
	SYSCALL(SENDMESSAGE, (unsigned int)sender, 0, 0);
}

static void getPID(void* arg, pcb_PTR sender){
	#ifdef DEBUG_SSI
	klog_print("SSI getPID\n");
	#endif
	
	if( arg==0 )
		SYSCALL(SENDMESSAGE, (unsigned int)sender, sender->p_pid, 0);
	else if( sender->p_parent==NULL )
		SYSCALL(SENDMESSAGE, (unsigned int)sender, 0, 0);
	else
		SYSCALL(SENDMESSAGE, (unsigned int)sender, sender->p_parent->p_pid, 0);
}

static void SSIRequest(pcb_t* sender, int service, void* arg){
	if(		service == CREATEPROCESS) createProcess(arg, sender);
	else if(service == TERMPROCESS	) killCall(arg, sender);
	else if(service == DOIO			) doIO(arg, sender);
	else if(service == GETTIME		) getTime(sender);
	else if(service == CLOCKWAIT	) waitForClock(sender);
	else if(service == GETSUPPORTPTR) getSupportStruct(sender);
	else if(service == GETPROCESSID	) getPID(arg, sender);
	else{
		klog_print("invalid SSI service\n");
		killProcess(sender, sender);
	}
}

void SSI(){
	ssi_payload_PTR payload = NULL;
	pcb_PTR sender;
	
	while(TRUE){
		sender = (pcb_PTR)SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int)(&payload), 0);
		SSIRequest(sender, payload->service_code, payload->arg);
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

	if(searchProcQ(&pcbFree_h, doomed) == doomed){
		// doomed pcb already killed
		return;
	}

	// detach from parent
	// remove from sibling list
	outChild(doomed);

	// kill all children recursively
	while(!emptyChild(doomed)){
		killProcess(removeChild(doomed), NULL);
	}

	//remove from any process queue
	if(current_process == doomed){
	}else if(searchProcQ(&readyQueue, doomed) == doomed){
		outAnyProcQ(doomed);
	}else if(searchProcQ(&receiveMessageQueue, doomed) == doomed){
		outAnyProcQ(doomed);
	}else if(searchProcQ(&pseudoClockQueue, doomed) == doomed){
		softBlockCount--;
		outAnyProcQ(doomed);
	}else{
		softBlockCount--;
		outDevQ(doomed);
	}

	#ifdef DEBUG
	klog_print("killing pcb ");
	klog_print_dec(doomed->p_pid);
	klog_print("\n");
	#endif

	process_count--;
	freePcb(doomed);
}
