#include "./headers/exceptions.h"

/* to be replaced in phase 3 */
void uTLB_RefillHandler(){
	setENTRYHI(0x80000000);
	setENTRYLO(0x00000000);
	TLBWR();
	LDST((state_t*) 0x0FFFF000);
}

void syscallHandler(void){
	//int userModeOn = currentProcess->p_s.status & USERPON
	/* information saved in registers:
	currentProcess->p_s.reg_a0 _a1 _a2 _a3 */

	/* assuming syscall will be successful */
	currentProcess->p_s.reg_v0 = 0;

	switch(currentProcess->p_s.reg_a0){
		case SENDMESSAGE:
			if(currentProcess->p_s.status & USERPON == 0){
				/*	PROGRAM TRAP EXCEPTION
					syscall only available in kernel mode */
			}
			pcb_PTR dest = currentProcess->p_s.reg_a1;
				//TODO
				msg_PTR msg = allocMsg();
			msg->m_payload = currentProcess->p_s.reg_a2;
				//TODO
		case RECEIVEMESSAGE:
			if(currentProcess->p_s.status & USERPON == 0){
				/*	PROGRAM TRAP EXCEPTION
					syscall only available in kernel mode */
			}
			//TODO
		case CREATEPROCESS:
			pcb_PTR newChild = allocPcb();
			if(newChild == NULL){
				// no free PCBs available
				currentProcess->p_s.reg_v0 = -1;
			}else{
				copyState(currentProcess->p_s.reg_a1, &newChild->p_s);
				//copySupportStruct(currentProcess->p_s.reg_a2, &newChild->p_supportStruct);
				
			}//TODO
		case TERMPROCESS:
			//TODO
		case DOIO:
			//TODO
		case GETTIME:
			//TODO
		case CLOCKWAIT:
			//TODO
		case GETSUPPORTPTR:
			//TODO
		case GETPROCESSID:
			//TODO
	}
}

/* redirects exceptions to the correct handler */
void exceptionHandler(void){
	/* the processor state at the time of the exception will
have been stored at the start of the BIOS Data Page */
	/* processor already set to kernel mode and disabled interrupts*/
	unsigned int cause = getCAUSE();
	unsigned int excCode = (cause & GETEXECCODE) >> CAUSESHIFT;
	
	switch(excCode){
		case IOINTERRUPTS:
			interruptHandler(cause);
		case TLBINVLDL:
			//TODO
		case TLBINVLDS:
			//TODO
		case SYSEXCEPTION:
			syscallHandler();
		case BREAKEXCEPTION:
			//TODO
		case PRIVINSTR:
			//TODO
	}

	
}