#include "./headers/exceptions.h"

extern struct list_head pcbFree_h;
extern pcb_PTR currentProcess;
extern pcb_PTR SSI;
extern struct list_head *readyQueue;
extern struct list_head *receiveMessageQueue;

/* to be replaced in phase 3 */
void uTLB_RefillHandler(void){
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
	
	klog_print("exeption not handled\n");
	breakPoint();
}

void syscallHandler(void){
	/* information saved in registers:
	currentProcess->p_s.reg_a0 reg_a1 reg_a2 reg_a3 */

	if(currentProcess->p_s.reg_a0 == SENDMESSAGE){
			if(currentProcess->p_s.status & USERPON == 0){
				/* reserved instruction PRIVINSTR
				syscall only available in kernel mode */
				programTrapHandler();
			}
			currentProcess->p_s.reg_v0 = sendMessage(currentProcess->p_s.reg_a1, currentProcess->p_s.reg_a2);
	}else if(currentProcess->p_s.reg_a0 == RECEIVEMESSAGE){
			if(currentProcess->p_s.status & USERPON == 0){
				/* reserved instruction PRIVINSTR
				syscall only available in kernel mode */
				programTrapHandler();
			}
			currentProcess->p_s.reg_v0 = receiveMessage(currentProcess->p_s.reg_a1, currentProcess->p_s.reg_a2);
	}

	state_t *savedState = (state_t *)BIOSDATAPAGE;
	savedState->pc_epc += WORDLEN;
	LDST(savedState);
}

/*
SYSCALL(SENDMESSAGE, (unsigned int)destination, (unsigned int)payload, 0);
*/
int sendMessage(pcb_PTR dest, unsigned int payload){
	if(dest == SSIADDRESS) dest = SSI;

	msg_PTR msg = allocMsg();
	if(msg == NULL) return MSGNOGOOD;
	msg->m_sender = currentProcess;
	msg->m_payload = payload;

	if(searchProcQ(&pcbFree_h, dest) == dest){
		return DEST_NOT_EXIST;
	}else{
		if(searchProcQ(receiveMessageQueue, dest) == dest)
			insertProcQ(readyQueue, outProcQ(receiveMessageQueue, dest));
		pushMessage(&dest->msg_inbox, msg);
		return 0;
	}
}

/*
SYSCALL(RECEIVEMESSAGE, (unsigned int)sender, (unsigned int)payload, 0);
*/
int receiveMessage(pcb_PTR sender, unsigned int payload){
	/* assuming ANYMESSAGE == NULL */
	msg_PTR msg = popMessage(&currentProcess->msg_inbox, sender);
	if(msg == NULL){
		copyState(BIOSDATAPAGE, &currentProcess->p_s);
		// TODO currentProcess->p_time += accumulated cpu time??
		insertProcQ(receiveMessageQueue, currentProcess);
		scheduler();
		return 0; // for compiler
		/* does work with specific sender */
	}else{
		*&payload = msg->m_payload;
		return msg->m_sender;
	}
}

void programTrapHandler(){
	/*	The Nucleus Program Trap exception handler should perform a standard Pass
		Up or Die operation using the GENERALEXCEPT index value. */
}