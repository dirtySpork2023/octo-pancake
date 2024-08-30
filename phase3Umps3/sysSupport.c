#include "./headers/sysSupport.h"

// exception codes 4-7, 9-12, syscalls(8)  are passed up to here
void generalExceptionHandler(){
	support_t* supportStruct = getSupportStruct();
	
	unsigned int cause = supportStruct->sup_exceptState[GENERALEXCEPT].cause;
	unsigned int excCode = (cause & GETEXECCODE) >> CAUSESHIFT;
	
	if(excCode == SYSEXCEPTION)
		usyscallHandler(supportStruct->sup_exceptState[GENERALEXCEPT]);
	else
		programTrapsHandler();
}

void usyscallHandler(state_t exst){
	if(exst.reg_a0 == SENDMSG){
		// USYS1
		// SYSCALL(SENDMSG, (unsigned int)destination, (unsigned int)payload, 0);
		if(exst.reg_a1 == PARENT)
			exst.reg_a1 = (unsigned int)current_process->p_parent;
		
		klog_print("U-sent\n");
		SYSCALL(SENDMESSAGE, exst.reg_a1, exst.reg_a2, 0);
	}else if(exst.reg_a0 == RECEIVEMSG){
		// USYS2
		// SYSCALL(RECEIVEMSG, (unsigned int)sender, (unsigned int)payload, 0);
		
		klog_print("U-rcvd\n");
		SYSCALL(RECEIVEMESSAGE, exst.reg_a1, exst.reg_a2, 0);
	}else{
		klog_print("ERR strange Usyscall\n");
	}
}

void programTrapsHandler(){
	SYSCALL(SENDMESSAGE, (unsigned int)swap_pcb, RELEASEMUTEX, 0);
	klog_print("support program trap\n");

	ssi_payload_t payload = {
		.service_code = TERMINATE,
		.arg = NULL,
	};
	SYSCALL(SENDMSG, PARENT, (unsigned int)(&payload), 0);
}
