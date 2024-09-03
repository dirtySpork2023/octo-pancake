#include "./headers/sst.h"

pcb_PTR child_pcb;
support_t childSupport;

// number of microseconds since startup
// SYSCALL(SENDMSG, PARENT, (unsigned int)&sst_payload, 0);
static unsigned int getTOD(){
	// low order word is sufficient, TODHIADDR is not considered	
	unsigned int time;
	STCK(time); // value of the low-order word of the TOD clock divided by the Time Scale
	return time;
}

// terminate SST and child after sending message to test
// SYSCALL(SENDMSG, PARENT, (unsigned int)&sst_payload, 0);
static void terminate(unsigned int asid){
	// notify test process
	SYSCALL(SENDMESSAGE, PARENT, 0, 0);
	
	// TODO clear tlb entries
	cleanSwapTable(asid);
	
	suicide();
	klog_print("I should be dead\n");
}

// write string to printer (or terminal) of same number as child ASID
// SYSCALL(SENDMSG, PARENT, (unsigned int)&sst_payload, 0);
static unsigned int writeString(sst_print_t* s, devreg_t* dev){
	#ifdef DEBUG_SST
	klog_print("out: ");
	klog_print(s->string);
	klog_print("\n");
	#endif

	unsigned int *cmdAddr, cmdValue;
	int isPrinter = (dev < (devreg_t *)TERM0ADDR);

	if(isPrinter)
		cmdAddr = &dev->dtp.command;
	else
		cmdAddr = &dev->term.transm_command;

	for(int i=0; i<s->length; i++){
		if(*s->string == '\0'){
			klog_print("ERR: sst print too long\n");
			break;
		}
		
		if(dev < (devreg_t *)TERM0ADDR){
			cmdValue = PRINTCHR;
			dev->dtp.data0 = (unsigned int)*s->string;
		}else{
			cmdValue = (((unsigned int)*s->string) << 8) | PRINTCHR;
		}

		ssi_do_io_t do_io = {
			.commandAddr = cmdAddr,
			.commandValue = cmdValue,
		};
		ssi_payload_t payload = {
			.service_code = DOIO,
			.arg = &do_io,
		};
    	unsigned int status;
		SYSCALL(SENDMESSAGE, SSIADDRESS, (unsigned int)(&payload), 0);
		SYSCALL(RECEIVEMESSAGE, SSIADDRESS, (unsigned int)(&status), 0);
	
		if((isPrinter && status!=1) || (status&TERMSTATMASK) != RECVD){
			klog_print("ERR: doIo status\n");
			klog_print_dec(status);
			breakPoint();
		}

		s->string++;
	}

	//send empty response to communicate completion of the write
	return 0;
}

static pcb_PTR initChild(unsigned int asid){
	state_t childState;
    STST(&childState);
	childState.reg_sp = USERSTACKTOP;
    childState.pc_epc = UPROCSTARTADDR;
	childState.reg_t9 = UPROCSTARTADDR;
	childState.status |= USERPON | IEPON | IMON | TEBITON;
	childState.entry_hi = asid << ASIDSHIFT;

	initSupportStruct(&childSupport, asid);

	return newProc(&childState, &childSupport);
}

static unsigned int SSTrequest(unsigned int asid, unsigned int service, void *arg){
	#ifdef DEBUG_SST
	klog_print("SST request ");
	klog_print_dec(service);
	klog_print("\n");
	#endif

	if(		service == GET_TOD) return getTOD();
	else if(service == TERMINATE) terminate(asid);
	else if(service == WRITEPRINTER) return writeString(arg, (devreg_t *)(PRNT0ADDR + (asid-1) * 0x10 ));
	else if(service == WRITETERMINAL) return writeString(arg, (devreg_t *)(TERM0ADDR + (asid-1) * 0x10 ));
	else{
		klog_print("ERR: invalid SST service\n");
		breakPoint();
	}
	return 0;
}

void SST(){
	unsigned int asid = (current_process->p_s.entry_hi & GETASID) >> ASIDSHIFT;
	pcb_PTR child_pcb = initChild(asid);
	
	// wait for service requests and manage them
	ssi_payload_PTR payload = NULL;
	unsigned int answer;
	while(TRUE){
		SYSCALL(RECEIVEMSG, (unsigned int)child_pcb, (unsigned int)(&payload), 0);
		answer = SSTrequest(asid, payload->service_code, payload->arg);
		SYSCALL(SENDMESSAGE, (unsigned int)child_pcb, answer, 0);
	}
}
