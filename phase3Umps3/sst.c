#include "headers/sst.h"

// system service thread
void SST(memaddr func){
	state_t childState;
	support_t childSupport;
	pcb_PTR child_pcb;
	
	// initialize the corresponding U-proc that will execute func
    STST(&childState);
    childState.reg_sp = childState.reg_sp - QPAGE;
    childState.pc_epc = func;
    childState.status |= IEPBITON | CAUSEINTMASK | TEBITON;

	/*
 support_t {
    int        sup_asid;                        process ID
    state_t    sup_exceptState[2];              old state exceptions
    context_t  sup_exceptContext[2];            new contexts for passing up
    pteEntry_t sup_privatePgTbl[USERPGTBLSIZE]; user page table
    struct list_head s_list;
} support_t;

	// TODO initialize support struct
	childSupport.sup_asid
	childSupport.sup_exceptState[2]
	childSupport.sup_exceptContext[2]
	childSupport.sup_privatePgTbl[USERPGTBLSIZE]
	*/

	ssi_create_process_t ssi_create_process = {
        .state = &childState,
        .support = &childSupport,
    };
    ssi_payload_t payload = {
        .service_code = CREATEPROCESS,
        .arg = &ssi_create_process,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)&payload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&child_pcb), 0);


	// wait for service requests and manage them
	sst_payload_t payload = NULL;
	unsigned int answer;

	while(TRUE){
		// I suppose requests can only be syncronous
	
		// should be support level message?	
		SYSCALL(RECEIVEMESSAGE, child_pcb, (unsigned int)(&payload), 0);
		
		if(payload.service_code == GET_TOD) answer = getTOD();
		else if(payload.service_code == TERMINATE) terminate();
		else if(payload.service_code == WRITEPRINTER) answer = writePrinter();
		else if(payload.service_code == WRITETERMINAL) answer = writeTerminal();
		else {
			klog_print("invalid SST service\n");
		}

		SYSCALL(SENDMESSAGE, child_pcb, answer, 0);
	}
}

// number of microseconds since startup
// SYSCALL(SENDMSG, PARENT, (unsigned int)&sst_payload, 0);
unsigned int getTOD(){
	unsigned int time;
	ssi_payload_t get_time_payload = {
		.service_code = GETTIME,
		.arg = NULL,
	};
	SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&get_time_payload), 0);
	SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&time), 0);
	
	return time;
}

// terminate SST and child after sending message to test
// SYSCALL(SENDMSG, PARENT, (unsigned int)&sst_payload, 0);
void terminate(){
	// TODO message to test

	ssi_payload_t term_process_payload = {
        .service_code = TERMPROCESS,
        .arg = (void *)NULL, //kill self
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&term_process_payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, 0, 0);
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
SYSCALL(SENDMSG, PARENT, (unsigned int)&sst_payload, 0);
Where str is a char* to the string to write and len is its length.
The mnemonic constant WRITEPRINTER has the value of 3.


*/
