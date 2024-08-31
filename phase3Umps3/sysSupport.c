#include "./headers/sysSupport.h"

// exception codes 4-7, 9-12, syscalls(8)  are passed up to here
void generalExceptionHandler(){
	support_t* supportStruct = getSupportStruct();
	unsigned int excCode = (supportStruct->sup_exceptState[GENERALEXCEPT].cause & GETEXECCODE) >> CAUSESHIFT;

	if(excCode == SYSEXCEPTION)
		usyscallHandler(supportStruct->sup_exceptState[GENERALEXCEPT]);
	else{
		programTrapsHandler();
	}
}

void usyscallHandler(state_t exst){
	if(exst.reg_a0 == SENDMSG){
		// USYS1
		// SYSCALL(SENDMSG, (unsigned int)destination, (unsigned int)payload, 0);

		pcb_PTR dest = exst.reg_a1==PARENT ? current_process->p_parent : (pcb_PTR)exst.reg_a1;
		ssi_payload_t *payload = (void *)exst.reg_a2;


		klog_print("U-send\n");
		SYSCALL(SENDMESSAGE, (unsigned int)dest, (unsigned int)payload, 0);
	}else if(exst.reg_a0 == RECEIVEMSG){
		// USYS2
		// SYSCALL(RECEIVEMSG, (unsigned int)sender, (unsigned int)payload, 0);
		
		klog_print("U-recv\n");
		SYSCALL(RECEIVEMESSAGE, exst.reg_a1, exst.reg_a2, 0);
	}else{
		klog_print("ERR strange Usyscall\n");
	}
}

void programTrapsHandler(){
	SYSCALL(SENDMESSAGE, (unsigned int)swap_pcb, RELEASEMUTEX, 0);

	ssi_payload_t payload = {
		.service_code = TERMINATE,
		.arg = (void *)NULL,
	};
	SYSCALL(SENDMSG, PARENT, (unsigned int)(&payload), 0);
}
