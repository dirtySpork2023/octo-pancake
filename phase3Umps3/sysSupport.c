#include "./headers/sysSupport.h"
#define EXST supportStruct.sup_exceptState[GENERALEXCEPT]


void klog_print();
void klog_print_dec();
void breakPoint();
extern pcb_PTR current_process;


// exception codes 4-7, 9-12, syscalls(8)  are passed up to here
void generalExceptionHandler(){
	ssi_payload_t payload = {
		service_code = GETSUPPORTPTR;
		arg = NULL;
	}
	support_t* supportStruct;
	SYSCALL(SENDMESSAGE, SSIADDRESS, payload, 0);
	SYSCALL(RECEIVEMESSAGE, SSIADDRESS, &supportStruct, 0);
	
	unsigned int cause = EXST.s_cause;
	unsigned int excCode = (cause & GETEXECCODE) >> CAUSESHIFT;

	if(excCode == SYSEXCEPTION)
		syscallHandler();
	else
		programTrapsHander();
}

void syscallHandler(){
	// destination can be PARENT, SST, (SSIADDRESS, pcb_ptr)
	
	if(EXST.reg_a0 == SENDMSG){
		// USYS1
		// SYSCALL(SENDMSG, (unsigned int)destination, (unsigned int)payload, 0);
		/*
		if(EXST.reg_a1 == PARENT)
			EXST.reg_a1 == currentProcess->p_parent ma si puo usare current process? */
		
		SYSCALL(SENDMESSAGE, EXST.reg_a1, EXST.reg_a2, 0);
	}else if(EXST.reg_a0 == RECEIVEMSG){
		// USYS2
		// SYSCALL(RECEIVEMSG, (unsigned int)sender, (unsigned int)payload, 0);
		SYSCALL(RECEIVEMESSAGE, EXST.reg_a1, EXST.reg_a2, 0);
	}else{
		klog_print("ERR strange Usyscall");
	}
	


}
void programTrapsHander(){}
