#include "./headers/exceptions.h"

/* to be replaced in phase 3 */
void uTLB_RefillHandler(){
	setENTRYHI(0x80000000);
	setENTRYLO(0x00000000);
	TLBWR();
	LDST((state_t*) 0x0FFFF000);
}

/* redirects exceptions to the correct handler */
void exceptionHandler(void){
	/* the processor state at the time of the exception will
	have been stored at the start of the BIOS Data Page */
	/* processor already set to kernel mode and disabled interrupts*/
	unsigned int cause = getCAUSE();
	unsigned int excCode = (cause & GETEXECCODE) >> CAUSESHIFT;

	if(excCode == IOINTERRUPTS)
		interruptHandler(cause);
	if(excCode == SYSEXCEPTION)
		syscallHandler();
	if(excCode <= 3) // codes 1-3
		{}//TLB exception handler, 
	if(excCode <= 12) // codes 4-7, 9-12
		programTrapHandler(); 
}

void syscallHandler(void){
	/* information saved in registers:
	currentProcess->p_s.reg_a0 reg_a1 reg_a2 reg_a3 */

	switch(currentProcess->p_s.reg_a0){
		case SENDMESSAGE:
			if(currentProcess->p_s.status & USERPON == 0){
				/* reserved instruction PRIVINSTR
				syscall only available in kernel mode */
				programTrapHandler();
			}
			sendMessage();
		case RECEIVEMESSAGE:
			if(currentProcess->p_s.status & USERPON == 0){
				/* reserved instruction PRIVINSTR
				syscall only available in kernel mode */
				programTrapHandler();
			}
			receiveMessage();
		case CREATEPROCESS:
			createProcess();
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

/* SYS1 */
void sendMessage(void){
	pcb_PTR dest = currentProcess->p_s.reg_a1;

	msg_PTR msg = allocMsg();
	msg->m_payload = currentProcess->p_s.reg_a2;
	//TODO
}

/* SYS2 */
void receiveMessage(void){
	//TODO
	// (receiveMessageQueue already created)
}

void createProcess(void){
	pcb_PTR newChild = allocPcb();
	if(newChild == NULL){
		/* no free PCBs available */
		currentProcess->p_s.reg_v0 = -1;
	}else{
		currentProcess->p_s.reg_v0 = 0;
		copyState(currentProcess->p_s.reg_a1, &newChild->p_s);
		//copySupportStruct(currentProcess->p_s.reg_a2, &newChild->p_supportStruct);
	}
}

void programTrapHandler(){
	/*	The Nucleus Program Trap exception handler should perform a standard Pass
		Up or Die operation using the GENERALEXCEPT index value. */
}