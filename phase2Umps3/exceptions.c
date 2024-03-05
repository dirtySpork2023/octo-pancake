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
			break;
		case RECEIVEMESSAGE:
			if(currentProcess->p_s.status & USERPON == 0){
				/* reserved instruction PRIVINSTR
				syscall only available in kernel mode */
				programTrapHandler();
			}
			receiveMessage();
			break;
		case CREATEPROCESS:
			createProcess();
			break;
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

	state_t *savedState = BIOSDATAPAGE;
	savedState->pc_epc += WORDLEN;
	LDST(savedState);
}

/* SYS1
SYSCALL(SENDMESSAGE, (unsigned int)destination, (unsigned int)payload, 0);
*/
void sendMessage(void){
	pcb_PTR dest = currentProcess->p_s.reg_a1;

	msg_PTR msg = allocMsg();
	if(msg == NULL)
		currentProcess->p_s.reg_v0 = MSGNOGOOD;
	msg->m_sender = currentProcess;
	msg->m_payload = currentProcess->p_s.reg_a2;

	if(searchProcQ(&pcbFree_h, dest) == dest){
		currentProcess->p_s.reg_v0 = DEST_NOT_EXIST;
	}else{
		if(searchProcQ(receiveMessageQueue, dest) == dest)
			insertProcQ(readyQueue, outProcQ(receiveMessageQueue, dest));
		pushMessage(&dest->msg_inbox, msg);
		currentProcess->p_s.reg_v0 = 0;
	}
}

/* SYS2
SYSCALL(RECEIVEMESSAGE, (unsigned int)sender, (unsigned int)payload, 0);
receive can be blocking
*/
void receiveMessage(void){
	pcb_PTR sender = currentProcess->p_s.reg_a1;

	/* assuming ANYSENDER == NULL */
	msg_PTR msg = popMessage(&currentProcess->msg_inbox, sender);
	if(msg == NULL){
		copyState(BIOSDATAPAGE, &currentProcess->p_s);
		//currentProcess->p_time += accumulated cpu time??
		insertProcQ(receiveMessageQueue, currentProcess);
		scheduler();
		/* does work with specific sender
		because PC is not updated when call is blocking */
	}else{
		*&currentProcess->p_s.reg_a2 = msg->m_payload;
		currentProcess->p_s.reg_v0 = 0;
	}
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