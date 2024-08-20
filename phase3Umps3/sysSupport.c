#include "./headers/sysSupport.h"

// exception codes 4-7, 9-12, syscalls(8)  are passed up to here
void generalExceptionHandler(){
	ssi_payload_t payload = {
		.service_code = GETSUPPORTPTR,
		.arg = NULL,
	};
	support_t* supportStruct;
	SYSCALL(SENDMESSAGE, SSIADDRESS, (unsigned int)&payload, 0);
	SYSCALL(RECEIVEMESSAGE, SSIADDRESS, &supportStruct, 0);
	
	unsigned int cause = supportStruct->sup_exceptState[GENERALEXCEPT].cause;
	unsigned int excCode = (cause & GETEXECCODE) >> CAUSESHIFT;

	if(excCode == SYSEXCEPTION)
		syscallHandler(supportStruct->sup_exceptState[GENERALEXCEPT]);
	else
		programTrapsHander();
}

void syscallHandler(state_t exst){
	if(exst.reg_a0 == SENDMSG){
		// USYS1
		// SYSCALL(SENDMSG, (unsigned int)destination, (unsigned int)payload, 0);
		if(exst.reg_a1 == PARENT)
			exst.reg_a1 = current_process->p_parent;
		
		SYSCALL(SENDMESSAGE, exst.reg_a1, exst.reg_a2, 0);
	}else if(exst.reg_a0 == RECEIVEMSG){
		// USYS2
		// SYSCALL(RECEIVEMSG, (unsigned int)sender, (unsigned int)payload, 0);
		
		SYSCALL(RECEIVEMESSAGE, exst.reg_a1, exst.reg_a2, 0);
	}else{
		klog_print("ERR strange Usyscall");
	}
}
void programTrapsHander(){}
