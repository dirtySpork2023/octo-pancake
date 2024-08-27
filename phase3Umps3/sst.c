#include "./headers/sst.h"

// number of microseconds since startup
// SYSCALL(SENDMSG, PARENT, (unsigned int)&sst_payload, 0);
static unsigned int getTOD(){
	// low order word is sufficient, TODHIADDR is not considered	
	unsigned int time;
	STCK(time); // value of the low-order word of the TOD clock divided by the Time Scale
	return time;
}

// write string to printer (or terminal) of same number as child ASID
// SYSCALL(SENDMSG, PARENT, (unsigned int)&sst_payload, 0);
static unsigned int writeString(sst_print_t* s, devreg_t* base){

	unsigned int *command = (unsigned int *)base + 3;
	if(command == &base->dtp.command) klog_print("use devreg_t");

	for(int i=0; i<s->length; i++){
		if(*s->string == EOS) klog_print("err, printing EOS\n");
		ssi_do_io_t do_io = {
			.commandAddr = &base->dtp.command,
			.commandValue = PRINTCHR | (((unsigned int)*s->string) << 8),
		};
		ssi_payload_t payload = {
			.service_code = DOIO,
			.arg = &do_io,
		};
    	unsigned int status;
		SYSCALL(SENDMESSAGE, SSIADDRESS, (unsigned int)(&payload), 0);
		SYSCALL(RECEIVEMESSAGE, SSIADDRESS, (unsigned int)(&status), 0);
		
		if ((status & TERMSTATMASK) != RECVD){
			klog_print("ERR: doIo status");
			breakPoint();
		}

		s->string++;
	}

	//send empty response to communicate completion of the write
	return 0;
}

void SST(){
	pcb_PTR child_pcb;
	unsigned int asid = (current_process->p_s.entry_hi & GETASID) >> ASIDSHIFT;
	klog_print("SST started\n");
	breakPoint();
	// initialize the corresponding U-proc
	state_t childState;
    STST(&childState);
	childState.reg_sp = USERSTACKTOP;
    childState.pc_epc = UPROCSTARTADDR;
	childState.reg_t9 = UPROCSTARTADDR;
	childState.status = ALLOFF | USERPON | IEPON | IMON | TEBITON;
	childState.entry_hi = asid << ASIDSHIFT;

	support_t childSupport;
	initSupportStruct(&childSupport, asid);
	breakPoint();	
	child_pcb = newProc(&childState, &childSupport);
	
	klog_print("child created\n");
	#ifdef DEBUG
	klog_print("SST");
	klog_print_dec(asid);
	klog_print(" ready");	
	#endif
	
	// wait for service requests and manage them
	ssi_payload_t payload;
	unsigned int answer;
	
	while(TRUE){
		// I suppose requests can only be syncronous
	
		SYSCALL(RECEIVEMESSAGE, (unsigned int)child_pcb, (unsigned int)(&payload), 0);
		
		if(payload.service_code == GET_TOD)
			answer = getTOD();
		else if(payload.service_code == TERMINATE)
			terminate();
		else if(payload.service_code == WRITEPRINTER){
			klog_print("prnt");
			klog_print_dec(asid);
			klog_print("\n");
			answer = writeString(payload.arg, (devreg_t *)(PRNT0ADDR + (asid-1) * 0x10 ));
		}else if(payload.service_code == WRITETERMINAL){
			klog_print("term");
			klog_print_dec(asid);
			klog_print("\n");
			answer = writeString(payload.arg, (devreg_t *)(TERM0ADDR + (asid-1) * 0x10 ));
		}else{
			klog_print("ERR: invalid SST service\n");
			breakPoint();
		}

		/*
		 * printer intLineNo = 6
		 * terminal intLineNo = 7
		 * DevNo = ASID (0-7)
		 *                /---   TERM0ADDR or PRNT0ADDR    ---\
		 * devaddrbiase = 0x1000.0054 + ((IntlineNo - 3) * 0x80) + (DevNo * 0x10)
		 */

		SYSCALL(SENDMESSAGE, (unsigned int)child_pcb, answer, 0);
	}
}

void terminate(){
	// notify test process
	SYSCALL(SENDMESSAGE, PARENT, 0, 0);

	suicide();
	klog_print("I should be dead\n");
}
