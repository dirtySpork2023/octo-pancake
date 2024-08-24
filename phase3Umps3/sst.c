#include "./headers/sst.h"

void SST(){
	pcb_PTR child_pcb;
	unsigned int asid = (current_process->p_s.entry_hi & GETASID) >> ASIDSHIFT;
	
	// initialize the corresponding U-proc
	state_t childState;
    // init other vars to zero
	childState.reg_sp = USERSTACKTOP;
    childState.pc_epc = UPROCSTARTADDR;
	childState.reg_t9 = UPROCSTARTADDR;
    STST(&childState.status);
	childState.status |= USERPON | IEPON | IMON | TEBITON;
	childState.entry_hi = asid << ASIDSHIFT;

	support_t childSupport;
	initSupportStruct(&childSupport, asid);
	child_pcb = newProc(&childState, &childSupport);
	
	
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
		else if(payload.service_code == WRITEPRINTER)
			answer = writeString(payload.arg, (devreg_t *)(PRNT0ADDR + 0/*ASID * 0x10*/ ));
		else if(payload.service_code == WRITETERMINAL)
			answer = writeString(payload.arg, (devreg_t *)(TERM0ADDR + 0/*ASID * 0x10*/ ));
		else {
			klog_print("invalid SST service\n");
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

unsigned int getTOD(){
	unsigned int *hiTOD = (memaddr *)TODHIADDR;
	if( *hiTOD != 0 ) klog_print("low order word not sufficient for getTOD\n");
		
	unsigned int time;
	STCK(time); // value of the low-order word of the TOD clock divided by the Time Scale
	return time;
}

void terminate(){
	// notify test process
	SYSCALL(SENDMESSAGE, PARENT, 0, 0);

	suicide();
	klog_print("I should be dead\n");
}

unsigned int writeString(sst_print_t* s, devreg_t* base){

	unsigned int *command = (unsigned int *)base + 3;
	if(command == &base->dtp.command) klog_print("use devreg_t");

	for(int i=0; i<s->length; i++){
		if(*s->string == EOS) klog_print("err, printing EOS\n");
		unsigned int value = PRINTCHR | (((unsigned int)*s->string) << 8);
		ssi_do_io_t do_io = {
			.commandAddr = command,
			.commandValue = value,
		};
		ssi_payload_t payload = {
			.service_code = DOIO,
			.arg = &do_io,
		};
    	unsigned int status;
		SYSCALL(SENDMESSAGE, SSIADDRESS, (unsigned int)(&payload), 0);
		SYSCALL(RECEIVEMESSAGE, SSIADDRESS, (unsigned int)(&status), 0);
		
		if ((status & TERMSTATMASK) != RECVD)
			PANIC();

		s->string++;
	}

	//send empty response to communicate completion of the write
	return 0;
}

/*
sst_print_t print_payload = {
	.length = len,
	.string = str,
};
ssi_payload_t sst_payload = {
	.service_code = WRITEPRINTER,
	.arg = &print_payload,
};
Where str is a char* to the string to write and len is its length.
The mnemonic constant WRITEPRINTER has the value of 3.


*/
