// system service thread
void SST(){
	// initialize the corresponding U-proc
	pcb_PTR child_pcb;
	
	
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
