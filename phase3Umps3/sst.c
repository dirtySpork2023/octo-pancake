#include "./headers/sst.h"

// system service thread
void SST(){
	pcb_PTR child_pcb;
	unsigned int asid = current_process->p_supportStruct->sup_asid;
	
	// initialize the corresponding U-proc
	state_t childState;
    STST(&childState); // copy of SST state
    childState.reg_sp = USERSTACKTOP;
    childState.pc_epc = UPROCSTARTADDR;
	childState.reg_t9 = UPROCSTARTADDR;
    childState.status |= USERPON | IEPBITON | CAUSEINTMASK | TEBITON;
	childState.entry_hi = asid << ASIDSHIFT;

	support_t childSupport;
	initSupportStruct(&childSupport, asid);
	newProc(&childState, &childSupport);


	// wait for service requests and manage them
	ssi_payload_t payload;
	unsigned int answer;

	while(TRUE){
		// I suppose requests can only be syncronous
	
		SYSCALL(RECEIVEMSG, child_pcb, (unsigned int)(&payload), 0);
		
		if(payload.service_code == GET_TOD)
			answer = getTOD();
		else if(payload.service_code == TERMINATE)
			terminate();
		else if(payload.service_code == WRITEPRINTER)
			answer = writeString(payload.arg, (devregtr *)(PRNT0ADDR + 0/*ASID * 0x10*/ ));
		else if(payload.service_code == WRITETERMINAL)
			answer = writeString(payload.arg, (devregtr *)(TERM0ADDR + 0/*ASID * 0x10*/ ));
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

		SYSCALL(SENDMSG, child_pcb, answer, 0);
	}
}

// number of microseconds since startup
// SYSCALL(SENDMSG, PARENT, (unsigned int)&sst_payload, 0);
unsigned int getTOD(){
	unsigned int *hiTOD = (memaddr *)TODHIADDR;
	if( *hiTOD != 0 ) klog_print("low order word not sufficient for getTOD\n");
		
	unsigned int time;
	STCK(time); // value of the low-order word of the TOD clock divided by the Time Scale
	return time;
}

// terminate SST and child after sending message to test
// SYSCALL(SENDMSG, PARENT, (unsigned int)&sst_payload, 0);
void terminate(){
	// notify test process
	SYSCALL(SENDMESSAGE, PARENT, 0, 0);

	suicide();
}

// write string to printer (or terminal) of same number as child ASID
// SYSCALL(SENDMSG, PARENT, (unsigned int)&sst_payload, 0);
unsigned int writeString(sst_print_t* s, devregtr* base){
	typedef unsigned int devregtr;

	devregtr *command = base + 3;
    devregtr status;
	
	for(int i=0; i<s->length; i++){
		if(*s->string == EOS) klog_print("printing EOS\n");
		devregtr value = PRINTCHR | (((devregtr)*s->string) << 8);
		ssi_do_io_t do_io = {
			.commandAddr = command,
			.commandValue = value,
		};
		ssi_payload_t payload = {
			.service_code = DOIO,
			.arg = &do_io,
		};
		SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
		SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&status), 0);
		
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
